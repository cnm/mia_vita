import pdb




import subprocess
from subprocess import PIPE
import os
import sys

receive = 0
total = 0
type = 0
i = 0

def getNext(str, find):
	flag = 0
	for word in str.strip().split('_'):
		if flag == 1 :
			return word
		if word == find :
			flag = 1
	return '---'

def startsWith(str, find):
	lens = len(find)
	if str[:lens] == find:
		return 1
	else:
		return 0
			
# For each directory in the current folder
for dirpath,dirnames,filenames in os.walk("./"):
	for dirname in dirnames:
		if startsWith(dirname, "distance"):
			# For each file
			for dirpath,dirname2,filename in os.walk(dirname):
				for filename1 in filename:
						p = subprocess.Popen("wc " + ("./" + dirname+ "/" + filename1) + " -l",
						          shell=True, stdout=PIPE, stderr=PIPE)
						stdoutdata, stderrdata = p.communicate()
						# For each line of the output
						for line in stdoutdata.strip().split("\n"):
							for word in line.strip().split(' '):
								if word != ' ' and word[0].isdigit() :
									if startsWith(filename1, "result_client") :
										receive += (int(word) - 1)
									elif startsWith(filename1, "result_server"):
										total += int(word) - 1
									elif startsWith(filename1, "result_midle"):
										continue
									
			print "With " + getNext(dirpath, "SPS") + " sps, routing " +  getNext(dirpath, "routing") + " at a distance of " + getNext(dirpath, "distance") + " m:"
			print "Were received " + str(receive) + " packages"
			print "Were sent " + str(total) + " packages"
			print "The percentage of packets received was " + str((int(receive) * 100 / int(total))) + " %"
			print ""
			receive = 0
			total = 0
