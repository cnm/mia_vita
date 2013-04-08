
Requirements
============

* Maven (tested with version 2, but should work on 3)

Install missing dependencies in local maven repo:
    * mvn install:install-file -Dfile=src/main/resources/seedCodec-1.0.8.jar  -DgroupId=seedCodec -DartifactId=edu.iris.dmc.seedcodec -Dversion=1.0.8 -Dpackaging=jar
    * mvn install:install-file -Dfile=src/main/resources/seisFile-1.5.2_localMod.jar -DgroupId=seisFile -DartifactId=edu.sc.seis.seisFile -Dversion=1.5.2 -Dpackaging=jar

Execute the java via maven
==========================

    * mvn exec:java

Create a executable jar
=======================

    * mvn install

creates target/myReader-1.0-SNAPSHOT-jar-with-dependencies.jar

Running all three axis
======================

To draw the images for all the three axis run: 

    * run_all.sh script

usage: help
===========
::
   --data-output <output path>   Filepath to write the data output
   --debug                       Prints mseed info as it reads the input file
   --help                        print this message and exit
   --mseed-path <mseed path>     Input mseed filepath
   --soft-line-limit <number>    Limits the number of mseed lines to process (only a soft limit, number of lines limit can be higher)


