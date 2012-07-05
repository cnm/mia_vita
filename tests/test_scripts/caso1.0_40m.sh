#!/bin/sh
#tenho de encontrar uma forma de verificar se o batman esta a funcionar

FILE=CASO1.0_40m_$(uname -n)-$(date +%T-%d-%m-%G)
iperf -s -u -t 10 -i 1 -y C > /root/test_scripts/RESULTADOS/$FILE
	
#enviar os dados de volta para o controlo

