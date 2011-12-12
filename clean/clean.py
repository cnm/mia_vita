from fabric.api import *

DO_AUTOREMOVE=True

def strip_modules():
    run('u=`uname -r`; strip -s `find /lib/modules/$u/ -name "*.ko"`')

def remove_stripped_modules():
    run('u=`uname -r`;rm -r /lib/modules/$u-stip')

def remove_apache():
    run('apt-get -y purge apache2')
    if DO_AUTOREMOVE:
        run('apt-get -y autoremove')

def remove_avahi():
    run('apt-get -y purge avahi-utils avahi-autoipd avahi-daemon')
    if DO_AUTOREMOVE:
        run('apt-get -y autoremove')

def remove_nfs():
    run('apt-get -y purge nfs-common libnfsidmap2')
    if DO_AUTOREMOVE:
        run('apt-get -y autoremove')

def do_reboot():
    run('reboot')

@parallel
def clean_arm():
    global DO_AUTOREMOVE
    strip_modules()
    remove_stripped_modules()
    DO_AUTOREMOVE=False
    remove_apache()
    remove_avahi()
    remove_nfs()
    run('apt-get -y autoremove')
    do_reboot()
