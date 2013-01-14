#!/usr/bin/python

import json
from pprint import pprint
import pdb
from time import sleep
import matplotlib.pyplot as plt

MV_CONST = 13900 / float(5605682)
MV_SENSOR = 2500 / float(pow(2,23))

def get_values():
    values = { 1: [], 2: [], 3: []}
    with open('miavita.json.2') as data_file:
        data = json.load(data_file)
        for k in data.keys():
            node_id = int(k.split(':')[0])
            seq = int(k.split(':')[1])
            c1 = data[k]["1"] * MV_SENSOR
            c2 = data[k]["2"] * MV_SENSOR * 0
            c3 = data[k]["3"] * MV_SENSOR * 0
            c4 = data[k]["4"] * MV_CONST

            values[node_id].append([c1, c2, c3, c4, seq])

    with open('miavita.json.3') as data_file:
        data = json.load(data_file)
        for k in data.keys():
            node_id = int(k.split(':')[0])
            seq = int(k.split(':')[1])
            c1 = data[k]["1"] * MV_SENSOR
#            c2 = (data[k]["2"] - 100000) * MV_CONST
            c2 = (data[k]["2"] - 00000) * MV_SENSOR
#            c3 = (data[k]["3"] + 50000) * MV_CONST
            c3 = (data[k]["3"]) * MV_SENSOR
            c4 = data[k]["4"] * MV_CONST

            values[node_id].append([c1, c2, c3, c4, seq])

    for k in values.keys():
        values[k].sort(key=lambda x: x[4])

    return values

def draw_values(val):
    plt.ion() # turn on interactive mode
    i = 1
    fig = plt.figure(1, figsize=(16,13))
    plt.draw()
    plt.clf()

    # Draw samples
    for node in range(2,4):
        if(i == 1):
            a = fig.add_subplot(3,1,i)
        else:
            a = fig.add_subplot(3,1,i)

        channel1 = [v[0] for v in val[node]]
        channel2 = [v[1] for v in val[node]]
        channel3 = [v[2] for v in val[node]]

        if(node==2):
            plt.plot(channel1, lw=0.8, color='b', label="z")
            plt.ylim(-2500, 2500)

        elif(node==3):
            plt.plot(channel1, lw=0.8, color='b', label="z")
            plt.plot(channel2, lw=0.8, color='r', label="e")
            plt.plot(channel3, lw=0.8, color='g', label="n")
            plt.ylim(-100, 100)
        plt.legend()

        if 1 == int(node):
            a.set_title("Sink Node: " + str(node))
        if 2 == int(node):
            a.set_title("UniAxial Sensor: " + str(node))
        if 3 == int(node):
            a.set_title("TriAxial Sensor: " + str(node))

        # plt.xlabel("Time")
        plt.ylabel("mV")

        i += 1

    # Draw channel 4
    for node in val.keys():
        if node == 1:
            continue
        a = fig.add_subplot(7,1,6)
        channel4 = [v[3] for v in val[node]]
        plt.plot(channel4, label="Node " + str(node))
        plt.ylim(11000, 14000)
        a.set_title("Battery ")
        plt.legend(loc='lower left')
        # plt.xlabel("Time")
        plt.ylabel("mV")

    #plt.xlim(0,1000)
    plt.draw()

#    plt.show()

def get_file():
    from subprocess import call
    url2 = "http://192.168.1.43/miavita.json.2"
    url3 = "http://192.168.1.43/miavita.json.3"
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

    if(l[index][3] < (4300000 * MV_CONST)):
        return True

    if(abs(l[index][0]) > (4000000 * MV_CONST)):
        return True
    if(abs(l[index][1]) > (4000000 * MV_CONST)):
        return True
    if(abs(l[index][2]) > (4000000 * MV_CONST)):
        return True

    # First see difference in nodes around
    to_compare = range(index + 1, min(index + 1 + n, len(l)))
    if(len(to_compare) < n):
        to_compare += range(max(0, index - (n - len(to_compare))), index)
    neighbour_diff = 0
    for i in to_compare:
        if (i + 1 == len(l)): continue
        neighbour_diff += abs(l[i][3] - l[i+1][3])

    # Now see point difference to neighbours
    point_dif = 0
    if(index == 0):
        point_dif = min(abs(l[index][3] - l[index + 1][3]), abs(l[index][3] - l[index + 2][3]))
    elif(index + 1 == len(l)):
        point_dif = min(abs(l[index][3] - l[index - 1][3]), abs(l[index][3] - l[index - 2][3]))
    else:
        point_dif = min(abs(l[index][3] - l[index + 1][3]), abs(l[index][3] - l[index - 1][3]))

  #  print "Point " + str(index) + " gave point/neighbour_diff: " + str(point_dif) + " / " + str(neighbour_diff) + "  " + str(l[index][3])

    beta = float(1.5)

    return point_dif > neighbour_diff * beta;

def average_neighbours(i, l, pos):
    if(len(l) < 3):
        return l[i] # Nothing I can do

    if(i == 0):
        return (l[i+1][pos] + l[i+2][pos]) / 2
    elif(i +1 == len(l)):
        return (l[i-1][pos] + l[i-2][pos]) / 2
    else:
        return (l[i-1][pos] + l[i+1][pos]) / 2

def clean_values(new, previous, last_seq, means):

    for k in new.keys():
        # Sort new[k] by sequence number
        new[k].sort(key=lambda x: x[4])

        i = 0

        for n in new[k]:
            seq_n_new = n[4]

            clone = n[:]
            if is_outlier(i, new[k]):
                #print "OUTLIER " + str(i) + "\t\t\t\t\t" + str(new[k][i][2]) + "\t\t" + str(means[k][2])
                clone[0] = means[k][0] 
                clone[1] = average_neighbours(i, new[k], 1)
                clone[2] = average_neighbours(i, new[k], 2)
                clone[3] = average_neighbours(i, new[k], 3)
            i += 1

            # Ignore repeated values
            if(seq_n_new <= last_seq[k]):
                continue

            # Fill missing values
            for x in xrange(last_seq[k] + 1, seq_n_new):
                previous[k].append([means[k][0],means[k][1],means[k][2],means[k][3],x])

            alpha = float(0.8)
            beta = float(0.1)

            means[k][3] = means[k][3] * alpha + clone[3] * (1 - alpha)

            #Make a copy of n
            clone[3] = means[k][3]

            last_seq[k] = seq_n_new
            previous[k].append(clone)

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
   try:
        run(previous, last_seq, means)
   except Exception as e:
        print "###################################"
        pprint "Except" + str(e)
        print "###################################"
        print
        sleep(2)
