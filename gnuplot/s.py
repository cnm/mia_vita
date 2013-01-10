#!/usr/bin/python

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
            node_id = int(k.split(':')[0])
            seq = int(k.split(':')[1])
            c1 = data[k]["1"]
            c2 = data[k]["2"]
            c3 = data[k]["3"]
            c4 = data[k]["4"]

            values[node_id].append([c1, c2, c3, c4, seq])

    with open('miavita.json.3') as data_file:
        data = json.load(data_file)
        for k in data.keys():
            node_id = int(k.split(':')[0])
            seq = int(k.split(':')[1])
            c1 = data[k]["1"]
            c2 = data[k]["2"]
            c3 = data[k]["3"]
            c4 = data[k]["4"]

            values[node_id].append([c1, c2, c3, c4, seq])
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
        plt.ylim(-1200000, 1200000)

        if 1 == int(node):
            a.set_title("Sink Node: " + str(node))
        if 2 == int(node):
            a.set_title("UniAxial Sensor: " + str(node))
        if 3 == int(node):
            a.set_title("TriAxial Sensor: " + str(node))

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

def limit_size(previous, maxi):
    for k in previous.keys():
        n = len(previous[k])
        if(n >= maxi):
            previous[k] = previous[k][n - maxi:] #Discard older elements
            print "Previous: " + str(n) + " - " + str(maxi)
        print

def is_outlier(index, l):
    if(not l):
        return False

    lenght = len(l)
    n = 10

    to_compare = range(index + 1, min(index + 1 + n, len(l)))

    if(len(to_compare) < n):
        to_compare += range(max(0, index - (n - len(to_compare))), index)

    total = 0
    for i in to_compare:
        if (i + 1 == len(l)): continue
#        print str(l[i][3]) + " " + str(l[i+1][3])
        total = abs(l[i][3] - l[i+1][3])

    beta = float(10000)

    if(index > 0):
        if total * beta < abs(l[index][3] - l[index -1][3]):
            print total
            return True
        else:
            return False
    else:
        if total * beta < abs(l[index][3] - l[index + 1][3]):
            print total
            return True
        else:
            return False

def clean_values(new, previous, last_seq, means):

    for k in new.keys():
        # Sort new[k] by sequence number
        new[k].sort(key=lambda x: x[4])

        i = 0

        for n in new[k]:
            seq_n_new = n[4]

            if is_outlier(i, new[k]):
                print "OUTLIER"
            i += 1

            # Ignore repeated values
            if(seq_n_new <= last_seq[k]):
                continue

            for x in xrange(last_seq[k] + 1, seq_n_new):
                previous[k].append([0,0,0,0,x])

            alpha = float(0.8)

            n[3] = means[k][3] = means[k][3] * alpha + n[3] * (1 - alpha)

            last_seq[k] = seq_n_new
            previous[k].append(n)

def run(previous, last_seq, means):
    while(True):
            get_file()
            val = get_values()
            clean_values(val, previous, last_seq, means)
            limit_size(previous, 1500)
            draw_values(previous)
            sleep(0.5)

previous = {1:[], 2:[], 3:[]}
last_seq = {1:0, 2:0, 3:0}
means = {1:[0,0,0,0], 2:[0,0,0,0], 3:[0,0,0,0]}
while True:
#   try:
        run(previous, last_seq, means)
#   except Exception as e:
        print "###################################"
        print "Except" + str(e)
        print "###################################"
        print
        sleep(2)
