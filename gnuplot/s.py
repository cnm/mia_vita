import json
from pprint import pprint
import pdb
from time import sleep
import matplotlib.pyplot as plt

def get_values():
    values = { 1: [], 2: [], 3: []}
    with open('miavita.json.4') as data_file:
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
    fig = plt.figure("Main")

    # Draw samples
    for node in val.keys():
        a = fig.add_subplot(2,2,i)
        channel1 = [v[0] for v in val[node]]
        channel2 = [v[1] for v in val[node]]
        channel3 = [v[2] for v in val[node]]

        plt.plot(channel1)
        plt.plot(channel2)
        plt.plot(channel3)
        a.set_title(str(node))
        i += 1

    # Draw channel 4
    for node in val.keys():
        a = fig.add_subplot(2,2,i)
        channel4 = [v[3] for v in val[node]]
        plt.plot(channel4)

    plt.draw()
    plt.show()

def get_file():
    from subprocess import call
    url = "http://tagus.inesc-id.pt/~jtrindade/miavita.json.4"
    call(["wget", url])

while(True):
    get_file()
    val = get_values()
    draw_values(val)
    sleep(10)

