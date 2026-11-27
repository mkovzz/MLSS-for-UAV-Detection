#main environment model that:
#   take in from the ephemeris to determine positions
#   takes in from sensor parameters to determine what/which satellites have sensors and what their efficiencies are
#   outputs events and satellite links to the run data store
#   manages interactions between all satellites:
#       outputs to a satellite the observations that it should be seeing
#       outputs to a satellite the message from another satellite
#       takes in from a satellite the message to send to another satellite
#       takes in from a satellite its event logs


#current edition - message passing and managing connections between satellites
# (any functions that handle messages getting sent from one satellite to another)

#use this for annotating the classes
from dataclasses import dataclass
import math
import heapq
from satellite import SatelliteObject, Message, Event, MsgType, Layer
#from "Ephemeris.h" import Ephemeris
from typing import Optional

import numpy as np


@dataclass
class Constellation:
    numSatellitesLEO : int
    numSatellitesMEO : int
    numSatellitesGEO : int
    shapeLEO : str





    #FILLER CODE TO HELP FUNCTION IN MEANTIME
    # Demo orbit parameters, used only by orbital_elements() for test ephemerides
    altLEO_m: float = 550e3
    incLEO_deg: float = 53.0
    planesLEO: Optional[int] = None  # default: round(sqrt(N))
    altMEO_m: float = 20_200e3
    incMEO_deg: float = 55.0
    planesMEO: int = 3
    altGEO_m: float = 35_786e3


    def total(self) -> int:
        return self.numSatellitesLEO + self.numSatellitesMEO + self.numSatellitesGEO

    def layer_ids(self) -> dict:
        """Satellite IDs per layer. IDs are contiguous: LEO first, then MEO, then GEO."""
        a = self.numSatellitesLEO
        b = a + self.numSatellitesMEO
        c = b + self.numSatellitesGEO
        return {Layer.LEO: list(range(0, a)),
                Layer.MEO: list(range(a, b)),
                Layer.GEO: list(range(b, c))}
    
    @staticmethod
    def _spread(n, planes, alt, inc_deg):
        out = []
        if n == 0:
            return out
        planes = max(1, min(planes, n))
        per_plane = math.ceil(n / planes)
        for k in range(n):
            p, slot = k % planes, k // planes
            raan = 2 * math.pi * p / planes
            phase = 2 * math.pi * slot / per_plane + math.pi * p / n   # offset planes so sats never coincide
            out.append((alt, math.radians(inc_deg), raan, phase))
        return out

    def orbital_elements(self) -> list:
        """(alt_m, inc_rad, raan_rad, phase_rad) per satellite, in ID order. Demo use only."""
        n = self.numSatellitesLEO
        planes = 1 if self.shapeLEO == "ring" else (self.planesLEO or max(1, round(math.sqrt(n))))
        el = self._spread(n, planes, self.altLEO_m, self.incLEO_deg)
        el += self._spread(self.numSatellitesMEO, self.planesMEO, self.altMEO_m, self.incMEO_deg)
        g = self.numSatellitesGEO
        el += [(self.altGEO_m, 0.0, 0.0, 2 * math.pi * k / g) for k in range(g)]
        return el
    ##end of code that is just filler code






    
@dataclass
class LinkState:
    """D-04 entry: existence, range, delay, margin, data rate for one satellite pair."""
    sat_a: int
    sat_b: int
    exists: bool
    distance_m: float
    delay_s: float
    margin_db: float
    rate_bps: float

@dataclass
class MessageEvent:
    #satellite message pass event format
    #[source (ID), destination (ID), type (such as if a target was found, if a handover occurred, etc), position of message in queue, delay, distance, data rate, time when sent at source, time when received]
    sourceID : int
    destID : int
    type : str
    posInQ : int = None
    delay : float = None
    distance : float = None
    rate : float = None
    timeSource : float = None
    timeDest : float = None
    status: str = "pending"

    def event(self) -> np.ndarray:
        return np.array([self.sourceID, self.destID, self.type, self.posInQ, self.delay, self.distance, self.rate, self.timeSource, self.timeDest], dtype=object)

#placeholder entity until combination with ephemeris C class


#FILLER CODE TO BE FILLED IN IN THE meANTIME FILLED
class Ephemeris:
    """PLACEHOLDER for the C++ Ephemeris. Environment only uses this interface:
         .times        -> (N,) array of stored times
         .n_entities   -> number of satellites
         .state_at(t)  -> (n_entities, 6) array of [x, y, z, vx, vy, vz]
    """
    def __init__(self, times, states):
        self.times = np.asarray(times, float)
        self.states = np.asarray(states, float)      # (N_times, N_entities, 6)

    @property
    def n_entities(self) -> int:
        return self.states.shape[1]

    def state_at(self, t: float) -> np.ndarray:
        if len(self.times) == 1:
            return self.states[0].copy()
        i = int(np.clip(np.searchsorted(self.times, t, side="right") - 1, 0, len(self.times) - 2))
        s = (t - self.times[i]) / (self.times[i + 1] - self.times[i])
        return (1 - s) * self.states[i] + s * self.states[i + 1]   # linear: fine for a placeholder

    @classmethod
    def circular(cls, elements, times) -> "Ephemeris":
        """Test ephemeris from circular orbits. elements: (alt_m, inc, raan, phase) in rad."""
        times = np.asarray(times, float)
        states = np.zeros((len(times), len(elements), 6))
        for m, (alt, inc, raan, phase) in enumerate(elements):
            a = 6000 + alt
            n = math.sqrt(4e14 / a**3)
            P = np.array([math.cos(raan), math.sin(raan), 0.0])
            Q = np.array([-math.sin(raan) * math.cos(inc), math.cos(raan) * math.cos(inc), math.sin(inc)])
            th = phase + n * times
            states[:, m, :3] = a * (np.cos(th)[:, None] * P + np.sin(th)[:, None] * Q)
            states[:, m, 3:] = a * n * (-np.sin(th)[:, None] * P + np.cos(th)[:, None] * Q)
        return cls(times, states)
#END OF FILLER CODE





    

class GroundTruthLogger:
    def __init__(self):
        self.MessageEvents: list[MessageEvent] = []   # D-09 (message pass events)
        self.SatelliteEvents: list[Event] = []              # D-06 (events as reported by satellites)

    def log_message_event(self, ev: MessageEvent) -> None:
        self.MessageEvents.append(ev)

    def log_satellite_events(self, events: list) -> list:
        """Store satellite-reported events; return their IDs as the 'logged ack'."""
        self.SatelliteEvents.extend(events)
        return [e.ID for e in events]


    
class EnvironmentOutputHandler:
    LINK_HEADER = ["t", "sat_a", "sat_b", "exists", "range_m", "delay_s", "margin_db", "rate_bps"]

    def __init__(self, logger: GroundTruthLogger):
        self.logger = logger
        self.linkRows: list = []

    def setUpLinks(self, t: float, states: list):
        for s in states:
            self.link_rows.append([t, s.sat_a, s.sat_b, s.exists, s.distance_m, s.delay_s, s.margin_db, s.rate_bps])

    def summary(self) -> dict:
        ev = self.logger.MessageEvents
        out = {"messages": len(ev)}
        for status in ("delivered", "dropped_no_link", "dropped_offline", "dropped_bad_dst"):
            out[status] = sum(1 for e in ev if e.status == status)
        out["satellite_events"] = len(self.logger.SatelliteEvents)
        out["link_rows"] = len(self.linkRows)
        return out




class Environment:
    def __init__(self, constArray : Constellation, ephemeris: Ephemeris, defaultRate: float = 1e7):
        self.const = constArray
        self.eph = ephemeris
        self.sats: dict[int, SatelliteObject] = {}
        self.defaultRate = defaultRate
        self.buildSatellites()
        self.logger = GroundTruthLogger()
        self.stagedMessages: dict[int, list] = {i: [] for i in range(constArray.total())}  #messages pulled out of a satellite's outbox but not yet handed to messagePassing
        self.time = float(ephemeris.times[0])
        self.positions = self.SatelliteHandler(self.eph, self.time)
        self.satelliteTransmitTimes: dict[int, list] = {i: [] for i in range(constArray.total())} #maps each satellite ID to a list of times, the times being when one of that satellite's queued messages finishes being transmitted
        self.inFlight: list = [] 
    def SatelliteHandler(self, eph, t):
        #establishes the positions of all bodies, to be used later when calculating link distances and viability
        #stores in a 2D array of the format [x, y, z, velocity]
                                           #[x, y, z, velocity]
                                           #   ...
        return eph.state_at(t)
    def linkState(self, s1, s2):
        #no need for implementation right now, need message passing completed
        return
    
    def linkViability(self, s1 : SatelliteObject, s2 : SatelliteObject):
        #determines whether two satellites can viably have a link
        #depends on distance, objects in the way, whether they are both online, etc
        #takes in from the ephemeris, which is handled by the SatelliteHandler function
        #returns a true or false
        #return self.linkState(s1, s2).exists
        return True

    def MessagePassing(self, s1 : SatelliteObject, target = None):
        #message passing function that takes the message from one satellite's inbox and directs it to its target
        #if its viable, it will be passed into the target's inbox
        #once the message is passed, the receipt of the transaction is stored as an event in the GroundTruthLogger,
        #along with the link details (what satellites were involved, what distance, delay, data rate, etc)
        ID = s1.ID
        keep = []
        for arr in self.stagedMessages[ID]:
            msg = Message.fromArray(arr)
            if target is not None and msg.destination != self.ID(target):
                keep.append(arr)
                continue
            self.messageTransmission(ID, msg, arr)
        self.stagedMessages[ID] = keep





    #FILLER CODE TO HELP IT RUN IN MEANTIME
    def buildSatellites(self) -> None:
        """Type Identifier: create each satellite and tell it which layer it is in."""
        for layer, ids in self.const.layer_ids().items():
            for sid in ids:
                sat = SatelliteObject(sid, layer, 1, 2, 3)
                self.sats[sid] = sat
        self.sats = dict(sorted(self.sats.items()))
        # for sat in self.sats.values():       # default peer tables: everyone in each layer
        #     sat.set_peers(dict(self.const.layer_ids()))


    def step(self, t: float) -> None:
        """Advance the environment to time t."""
        self.time = float(t)
        self.positions = self.SatelliteHandler(self.eph, t)
        self.deliverDue(t)                  # messages that have arrived by t
        self.collectOutputs()               # new messages/events produced by satellites
        for sat in self.sats.values():
            self.MessagePassing(sat)        # transmit them (schedule delivery)

    def deliverDue(self, t: float) -> None:
        """Hand every in-flight message whose arrival time is <= t to its destination satellite."""
        while self.inFlight and self.inFlight[0][0] <= t + 1e-12:
            _, _, dst, arr, ev = heapq.heappop(self.inFlight)
            if not self.online[dst]:
                ev.status, ev.timeDest = "dropped_offline", math.nan
                self.logger.log_message_event(ev)
                continue
            self.sats[dst].rcv_msg_from_env(arr)        # destination's D-05 in
            ev.status = "delivered"
            self.logger.log_message_event(ev)  

    def run(self, t_end: float, dt: float, before_step=None) -> None:
        """Step from the current time to t_end. before_step(env, t) is an optional hook."""
        t = self.time
        while t <= t_end + 1e-9:
            if before_step:
                before_step(self, t)
            self.step(t)
            t += dt
    def collectOutputs(self) -> None:
        """Pull messages (D-05 out) and events (D-06) from every satellite; send back the logged ack."""
        for sid, sat in self.sats.items():
            msgs, events = sat.poll_outputs()
            if events:
                ids = self.logger.log_satellite_events(events)
                sat.rcvAck(ids)
            self.stagedMessages[sid].extend(msgs)

    def flush(self) -> None:
        """Deliver every message still in flight (call after the last step)."""
        if self.inFlight:
            self.deliverDue(max(f[0] for f in self.inFlight))
            self.collectOutputs()
    #END OF FILLER CODE



    def messageTransmission(self, src: int, msg: Message, arr: np.ndarray) -> None:
        t = self.time
        tname = MsgType(msg.type).name
        
        ev = MessageEvent(src, msg.destination, tname, timeSource=t)


        #case for if destination is impossible
        if msg.destination not in self.sats or msg.destination == src:
            ev.status = "dropped_bad_dst"
            self.logger.log_message_event(ev)
            return


        #case for if link is non-functioning
        st = self.linkState(src, msg.destination)
        # ev.distance = st.distance_m
        # if not st.exists:
        #     ev.status = ("dropped_offline" if not (self.online[src] and self.online[msg.dst])
        #                  else "dropped_no_link")
        #     self.logger.log_message_event(ev)
        #     return


        queue = self.satelliteTransmitTimes[src]                    # finish times of messages still transmitting
        queue[:] = [f for f in queue if f > t]
        ev.posInQ = len(queue)
        start = queue[-1] if queue else t
        finish = start + arr.size * 64 / st.rate_bps  #assuming 64 bits
        queue.append(finish)
        arrival = finish + st.delay_s

        ev.rate, ev.delay, ev.timeDest = st.rate_bps, arrival - t, arrival
        out = arr.copy()
        out[4] = arrival                                # receiver sees its arrival time in the header
        heapq.heappush(self._in_flight, (arrival, next(self._tick), msg.dst, out, ev))


if __name__ == "__main__" : 
    const = Constellation(numSatellitesLEO=6, numSatellitesMEO=0, numSatellitesGEO=0, shapeLEO="ring")
    times = np.arange(0.0, 301.0, 10.0)
    eph = Ephemeris.circular(const.orbital_elements(), times)
    env = Environment(const, eph, defaultRate=1e4)



    # satellites 0 and 2 put messages in their outboxes (sat 0 sends three in a burst)
    track = [1, 7.0e6, 0.0, 0.0, 0.0, 7500.0, 0.0, 1.0]       # [id, x, y, z, vx, vy, vz, confirmed]
    env.sats[0].sendMsgToEnv(dict(msg_type=MsgType.TRACK_REPORT, payload=track), target=3)
    env.sats[0].sendMsgToEnv(dict(msg_type=MsgType.COVERAGE_REQ, payload=[7.0e6, 0, 0, 30.0]), target=3)
    env.sats[0].sendMsgToEnv(dict(msg_type=MsgType.TRACK_REPORT, payload=track), target=1)
    env.sats[2].sendMsgToEnv(dict(msg_type=MsgType.COVERAGE_REQ, payload=[7.0e6, 0, 0, 10.0]), target=4)

    print("Outbox sizes before the run:", {i: len(s.router.outbox) for i, s in env.sats.items()})


    env.run(t_end=20.0, dt=10.0)
    env.flush()



    print("\nGround-truth message log (D-09):")
    print(f"{'src':>3} {'dst':>3} {'type':<13} {'posInQ':>6} {'sent(s)':>8} {'arrive(s)':>9} "
          f"{'dist(km)':>9} {'rate(bps)':>9} {'delay(s)':>8}  status")
    for e in env.logger.MessageEvents:
        print(f"{e.sourceID:>3} {e.destID:>3} {e.type:<13} {e.posInQ:>6} {e.timeSource:>8.2f} "
              f"{e.timeDest:>9.4f} {e.distance/1e3:>9.1f} {e.rate:>9.0f} {e.delay:>8.4f}  {e.status}")

    print("\nWhat the receiving satellites ended up with:")
    for sid in (1, 3, 4):
        s = env.sats[sid]
        got = [ev for ev in s.mem.event_log if ev.event_type == EventType.MSG_RECEIVED]
        print(f"  sat {sid}: MSG_RECEIVED events={len(got)}, tracks held={len(s.fusion.tracks)}, "
              f"coverage requests={len(s.mem.read('coverage_requests'))}, inbox left={len(s.router.inbox)}")
    print("\nOutbox sizes after the run:", {i: len(s.router.outbox) for i, s in env.sats.items()})
    print("Summary:", env.output.summary())
