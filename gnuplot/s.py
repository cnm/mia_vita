import json
from pprint import pprint
import pdb
from time import sleep
import matplotlib.pyplot as plt

def get_values():
    values = { 1: [], 2: [], 3: []}
    with open('miavita.json.2') as data_file:
        data = json.load(data_file)
        for k in data.keys():
            node_id = data[k]["node_id"]
            c1 = data[k]["sample_1"]
            c2 = data[k]["sample_2"]
            c3 = data[k]["sample_3"]
            c4 = data[k]["sample_4"]

            values[node_id].append((c1, c2, c3, c4))

    with open('miavita.json.3') as data_file:
        data = json.load(data_file)
        for k in data.keys():
            node_id = data[k]["node_id"]
            c1 = data[k]["sample_1"]
            c2 = data[k]["sample_2"]
            c3 = data[k]["sample_3"]
            c4 = data[k]["sample_4"]

            values[node_id].append((c1, c2, c3, c4))
    return values

def draw_values(val):
    plt.ion() # turn on interactive mode
    i = 1
    fig = plt.figure(1, figsize=(15,10))
    plt.draw()
    plt.clf()

    # Draw samples
    for node in val.keys():
        a = fig.add_subplot(2,2,i)
        channel1 = [v[0] for v in val[node]]
        channel2 = [v[1] for v in val[node]]
        channel3 = [v[2] for v in val[node]]

        plt.plot(channel1, label="x")
        plt.plot(channel2, label="y")
        plt.plot(channel3, label="z")
        plt.ylim(-2500000, 2500000)
        a.set_title("Node: " + str(node))
        i += 1

    # Draw channel 4
    for node in val.keys():
        a = fig.add_subplot(2,2,i)
        channel4 = [v[3] for v in val[node]]
        plt.plot(channel4)
        plt.ylim(4000000, 6000000)
        a.set_title("Battery ")

    #plt.xlim(0,1000)
    plt.draw()

#    plt.show()

def get_file():
    from subprocess import call
    url2 = "http://192.168.2.43/miavita/miavita.json.2"
    url3 = "http://192.168.2.43/miavita/miavita.json.3"
    call(["rm", 'miavita.json.2'])
    call(["rm", 'miavita.json.3'])
    call(["wget", url2])
    call(["wget", url3])

def join_values(previous, val):
    for k in previous.keys():
        previous[k] += (val[k])

def limit_size(previous, maxi):
    for k in previous.keys():
        n = len(previous[k])
        if(n > maxi):
            previous[k] = previous[k][n - maxi:] #Discard older elements
            print "Previous: " + str(n) + " - " + str(maxi)
        print "Len result" + str(len(previous[k]))
        print

def run(previous):
    while(True):
            get_file()
            val = get_values()
            join_values(previous, val)
            limit_size(previous, 1500)
            draw_values(previous)
            sleep(5)

previous = {1:[], 2:[], 3:[]}
while True:
    try:
        print "Weird: " + str(len(previous[1]))
        run(previous)
    except Exception as e:
        print "###################################"
        print "Except" + str(e)
        print "###################################"
        print
        sleep(2)
