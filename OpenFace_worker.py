import time
import zmq

def zmq_init(zmq_connectstring):
    global context, consumer_receiver
    context = zmq.Context()
    consumer_receiver = context.socket(zmq.PULL)
    print("connecting to zmq... ", end="")
    consumer_receiver.connect(zmq_connectstring)
    time.sleep(0.5)
    print("done")

def zmq_consumer():
    global context, consumer_receiver
    while True:
        try:
            work = consumer_receiver.recv_multipart(copy=False)
            time.sleep(0.01)
            return str(work[0])
        except zmq.ZMQError:
            return None

