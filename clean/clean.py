from fabric.api import run

def strip_modules():
    run('strip -s `find /lib/modules/2.6.24.4/ -name "*.ko"`')

def clean_arm():
    run('ls banana')
