#Satellite object that 
    #takes in events and logged acknowledges from the environment model
    #takes in messages from the environment model
    #takes in sensor observations from the environment model (such as whether or not a satellite is able to see a target)
    #output messages to the environment model

#These messages would be an array of numbers showing target information (positional) and requests (like handoff, coverage, etc)

import enum
import numpy as np
from collections import deque
import itertools
from dataclasses import dataclass

#these classes might be useless too, could a simpler approach work???
#could these be defined functions in main or something?
@dataclass
class Message:
    #details what types each of the elements of a message can have\
    type: int
    source: int
    destination: int
    seq: int
    time: float
    payload: np.ndarray

    def to_array(self) -> np.ndarray:
        head = [self.type, self.source, self.destination, self.seq, self.time, len(self.payload)]
        return np.concatenate([np.asarray(head, float), np.asarray(self.payload, float)])
    
    def fromArray(arr):
        arr = np.asarray(arr, float)
        n = int(arr[5])
        return Message(int(arr[0]), int(arr[1]), int(arr[2]), int(arr[3]), float(arr[4]), arr[6:6 + n])


         
@dataclass
class MsgType(enum.IntEnum):
    DEFAULT = 0
    TRACK_REPORT = 1   # payload: [track_id, x, y, z, vx, vy, vz, confirmed]
    HANDOFF_REQ = 2    # payload: [track_id, x, y, z, vx, vy, vz, confirmed]
    COVERAGE_REQ = 3   # payload: [x, y, z, duration_s]
    ACK = 4            # payload: [acked_seq] 

@dataclass
class Event:
    """D-06 entry. `event_id` is what the ground-truth logger acks."""
    ID: int
    t: float
    SatelliteID: int
    event_type: int
    data: tuple = ()


# class OffloadHandler:

class Mem:
    def __init__(self, sat_ID: int):
        self.sat_ID = sat_ID
        
       
        self.event_log: list[Event] = []          # full local copy
        self._unsent: deque[Event] = deque()      # waiting for env pickup
        self._pending_ack: dict[int, Event] = {}  # sent but not yet acked
        self._ids = itertools.count(1)

    def logEvent(self, t: float, etype, *data) -> Event:
        ev = Event(next(self._ids), t, self.sat_ID, int(etype), tuple(data))
        self.event_log.append(ev)
        self._unsent.append(ev)
        return ev


    #FILLER CODE _ CHECK LATER
    def pop_unsent_events(self) -> list[Event]:
        out = list(self._unsent)
        self._unsent.clear()
        for ev in out:
            self._pending_ack[ev.ID] = ev
        return out

    def handleAck(self, event_ids):
            n = 0
            for eventID in np.atleast_1d(event_ids):
                if self._pending_ack.pop(int(eventID), None) is not None:
                    n+=1
            return n




#unnecessary
class Layer(enum.IntEnum):
    LEO = 0
    MEO = 1
    GEO = 2

#manages intersatellite connections, like what satellites it is connected to , etc
class Router:
    def __init__(self, ID):
        self.ID = ID
        self.layerSats: dict[Layer, list[int]] = {l: [] for l in Layer}
        self.inbox: deque[Message] = deque()
        self.outbox: deque[Message] = deque()
        self._pending_ack: dict[int, Event] = {}
        #ID for each packets, defined as its sequence
        self.genseq = itertools.count(1)


    #list of linked satellites held in dict form
    def setLayerSats(self, layerSats: dict):
        for layer, ID in layerSats.items():
            #at the appropriate layer number in this satellite's layerSats dict
            #it fills in all satellites that have the same layer number
            self.layerSats[Layer(layer)] = [i for i in ID if i !=self.ID]

    def enqueue(self, type, destination, time, payload):
        msg = Message(int(type), self.ID, int(destination), next(self.genseq), time, payload)
        self.outbox.append(msg)
        return msg
    #def selectDestination(self, target)



    #FILLER CODE __ CHECK LATER
    def drain_outbox(self) -> list[Message]:
        out = list(self.outbox)
        self.outbox.clear()
        return out

    
    
# class Fusion:
#     # 
# class Sensing:
#     #distance, FOV, etc






class SatelliteObject:
    def __init__(self, ID, layer, FOV, maxRange, time):
        self.ID = ID
        self.layer = layer
        self.FOV = FOV
        self.maxRange = maxRange
        self.time = 0.0  #internal clock, for messages
        self.mem = Mem(ID)
        self.router=Router(ID)

    def sendMsgToEnv(self, header, target):
        #function that preps a message to send to a specific satellite
        #this has to pass through the environment model to verify link existence and if it is possible
        #it is an array of numbers detailing 
        msg = self.router.enqueue(header["type"], target, self.time, header.get("payload", []))
        self.mem.logEvent(self.time, msg.destination, msg.type, msg.seq)
        return

    def rcvMsgFromEnv(self, msgArray):
        #function that waits to receive message from satellite model
        #functions as an interrupt

        msg = Message.fromArray(msgArray)
        self.router.inbox.append(msg)
        return

    def rcvSensor(self, observation):
        #function that takes in a regular reading from the environment model
        #detailing whether or not its infrared sensor is capable of detecting any targets on Earth
        #from its cone of view
        self.time = observation.time
        self.fusion.update(observation)

    def rcvAck(self, event):
        #function that waits for an ack from the environment model ground-truth logger
        #this will work with the rcvSensor to tell the satellite whether or not the satellite can sense a target
        return self.mem.handleAck(event)


    #FILLER CODE -- DELETE LATER
    def poll_outputs(self) -> tuple[list[np.ndarray], list[Event]]:
        """Env collects D-05 out messages (as arrays) and D-06 events."""
        msgs = [m.to_array() for m in self.router.drain_outbox()]
        return msgs, self.mem.pop_unsent_events()


    #might be entirely useless because when instantiating layers, that can be defined in the object declaration
    def layer():
        #function that takes in from the Type Identifier function of the environment model to confirm what layer this satellite is in
        return