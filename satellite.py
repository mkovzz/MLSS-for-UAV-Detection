#Satellite object that 
    #takes in events and logged acknowledges from the environment model
    #takes in messages from the environment model
    #takes in sensor observations from the environment model (such as whether or not a satellite is able to see a target)
    #output messages to the environment model

#These messages would be an array of numbers showing target information (positional) and requests (like handoff, coverage, etc)


Class Message:
    #details what types each of the elements of a message can have

Class OffloadHandler:

Class Mem:
Class Router:

Class Fusion:
    # 
Class Sensing:
    #distance, FOV, etc
class SatelliteObject:
    def __init__(self, ID, layer, FOV, maxRange, time):
        self.ID = ID
        self.layer = layer
        self.FOV = FOV
        self.maxRange = maxRange
        self.time = 0.0  #internal clock, for messages

def sendMsgToEnv(self, header, target):
    #function that preps a message to send to a specific satellite
    #this has to pass through the environment model to verify link existence and if it is possible
    #it is an array of numbers detailing 
    msg = self.router.enqueue(header["type"], target, self.time, header.get("payload", []))
    self.mem.logEvent(self.time, msg.destination, msg.type, msg.sequence)
    return

def rcvMsgFromEnv(self, msgArray):
    #function that waits to receive message from satellite model
    #functions as an interrupt

    msg = Message.fromArray(msgArray)
    self.router.append(msg)
    return

def rcvSensor(self, observation):
    #function that takes in a regular reading from the environment model
    #detailing whether or not its infrared sensor is capable of detecting any targets on Earth
    #from its cone of view
    self.time = observation.time
    self.fusion.update(observation)

    return

def rcvAck():
    #function that waits for an ack from the environment model ground-truth logger
    #this will work with the rcvSensor to tell the satellite whether or not the satellite can sense a target
    return

def layer():
    #function that takes in from the Type Identifier function of the environment model to confirm what layer this satellite is in
    return