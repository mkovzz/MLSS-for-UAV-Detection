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

import heapq
from satellite import SatelliteObject, Message, Event, MsgType
from "Ephemeris.h" import Ephemeris

import numpy as np


#dataclasses
class Constellation:
    numSatellitesLEO : int
    numSatellitesMEO : int
    numSatellitesGEO : int
    shapeLEO : str


class LinkState:
    """D-04 entry: existence, range, delay, margin, data rate for one satellite pair."""
    sat_a: int
    sat_b: int
    exists: bool
    distance_m: float
    delay_s: float
    margin_db: float
    rate_bps: float


class MessageEvent:
    #satellite message pass event format
    #[source (ID), destination (ID), type (such as if a target was found, if a handover occurred, etc), position of message in queue, delay, distance, data rate, time when sent at source, time when received]
    sourceID : int
    destID : int
    type : str
    posInQ : int
    delay : float
    distance : float
    rate : float
    timeSource : float
    timeDest : float
    status: str = "pending"

    def event(self) -> np.ndarray:
        return np.array([self.sourceID, self.destID, self.type, self.posInQ, self.delay, self.distance, self.rate, self.timeSource, self.timeDest], dtype=object)


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

class Environment:
    def __init__(self, constArray : Constellation, ephemeris: Ephemeris):
        self.const = Constellation()
        self.positions = self.SatelliteHandler(ephemeris)
        self.stagedMessages: dict[int, list] = {i: [] for i in range(constArray.total)}  #messages pulled out of a satellite's outbox but not yet handed to messagePassing
        self.time = float(ephemeris.times[0])
        self.satelliteTransmitTimes: dict[int, list] = {i: [] for i in range(constArray.total)} #maps each satellite ID to a list of times, the times being when one of that satellite's queued messages finishes being transmitted

    def SatelliteHandler(self, eph):
        #establishes the positions of all bodies, to be used later when calculating link distances and viability
        #stores in a 2D array of the format [x, y, z, velocity]
                                           #[x, y, z, velocity]
                                           #   ...
        return
    def linkState(self, s1, s2):
        #no need for implementation right now, need message passing completed
        return
    def linkViability(self, s1 : SatelliteObject, s2 : SatelliteObject):
        #determines whether two satellites can viably have a link
        #depends on distance, objects in the way, whether they are both online, etc
        #takes in from the ephemeris, which is handled by the SatelliteHandler function
        #returns a true or false
        return self.linkState(s1, s2).exists

    def MessagePassing(self, s1 : SatelliteObject, target):
        #message passing function that takes the message from one satellite's inbox and directs it to its target
        #if its viable, it will be passed into the target's inbox
        #once the message is passed, the receipt of the transaction is stored as an event in the GroundTruthLogger,
        #along with the link details (what satellites were involved, what distance, delay, data rate, etc)
        ID = self.ID(s1)
        keep = []
        for arr in self.stagedMessages[ID]:
            msg = Message.from_array(arr)
            if target is not None and msg.dst != self._sid(target):
                keep.append(arr)
                continue
            self.messageTransmission(ID, msg, arr)
        self.stagedMessages[ID] = keep






    def messageTransmission(self, src: int, msg: Message, arr: np.ndarray) -> None:
        t = self.time
        tname = MsgType(msg.msg_type).name
        
        ev = MessageEvent(src, msg.dst, tname, timeSource=t)


        #case for if destination is impossible
        if msg.dst not in self.sats or msg.dst == src:
            ev.status = "dropped_bad_dst"
            self.logger.log_message_event(ev)
            return


        #case for if link is non-functioning
        st = self.linkState(src, msg.dst)
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
