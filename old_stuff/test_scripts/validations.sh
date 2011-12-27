EXEC=$1

#Check if the module for batman exists
if [ ! -e "batman-adv.ko" ]
then
    echo "Missing batman module file 'batman-adv.ko'"
    return 1
fi

#Verify if exec file is executable
if [ ! -e ${EXEC} ]
then
    echo "Passed exec file is not executable"
    return 1
fi
