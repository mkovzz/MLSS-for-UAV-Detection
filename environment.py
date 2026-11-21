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

from satellite import SatelliteObject
import numpy as np
class Constellation:
    numSatellitesLEO : int
    numSatellitesMEO : int
    numSatellitesGEO : int
    shapeLEO : str

class GroundTruthLogger:
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
    event : np.array[int, int, str, int, float, float, float, float, float]
    def __int__(self, sourceID : int,
                destID : int,
                type : str,
                posInQ : int,
                delay : float,
                distance : float,
                rate : float,
                timeSource : float,
                timeDest : float):
        self.sourceID = sourceID
        self.destID = destID
        self.type = type
        self.posInQ = posInQ
        self.delay = delay
        self.distance = distance
        self.rate = rate
        self.timeSource = timeSource
        self.timeDest = timeDest

class EnvironmentOutputHandler:

class Environment:
    def __init__(self, constArray : Constellation, ephemeris):
        self.const = Constellation()
        self.positions = self.SatelliteHandler(ephemeris)

    def SatelliteHandler(eph):
        #establishes the positions of all bodies, to be used later when calculating link distances and viability
        #stores in a 2D array of the format [x, y, z, velocity]
                                           #[x, y, z, velocity]
                                           #   ...
    def linkViability(s1 : SatelliteObject, s2 : SatelliteObject):
        #determines whether two satellites can viably have a link
        #depends on distance, objects in the way, whether they are both online, etc
        #takes in from the ephemeris, which is handled by the SatelliteHandler function
        #returns a true or false

    def MessagePassing(s1 : SatelliteObject, target):
        #message passing function that takes the message from one satellite's inbox and directs it to its target
        #if its viable, it will be passed into the target's inbox
        #once the message is passed, the receipt of the transaction is stored as an event in the GroundTruthLogger,
        #along with the link details (what satellites were involved, what distance, delay, data rate, etc)

