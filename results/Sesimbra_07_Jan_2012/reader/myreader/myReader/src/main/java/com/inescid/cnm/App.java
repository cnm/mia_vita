package com.inescid.cnm;

import java.io.BufferedInputStream;
import java.io.DataInput;
import java.io.DataInputStream;
import java.io.EOFException;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.PrintWriter;

import edu.iris.dmc.seedcodec.Codec;
import edu.iris.dmc.seedcodec.CodecException;
import edu.iris.dmc.seedcodec.DecompressedData;
import edu.iris.dmc.seedcodec.UnsupportedCompressionType;
import edu.sc.seis.seisFile.mseed.Blockette1000;
import edu.sc.seis.seisFile.mseed.DataRecord;
import edu.sc.seis.seisFile.mseed.SeedFormatException;
import edu.sc.seis.seisFile.mseed.SeedRecord;

/**
 * Hello world!
 *
 */
public class App {

    public static void main(String[] args) {
        System.out.println("Hello World!");
        String filename = "/home/workspace/mia_vita/results/Sesimbra_07_Jan_2012/theirs/IP.PSES..BHN.D.2013.038";

        try {
            DataInput dis = new DataInputStream(new BufferedInputStream(
                    new FileInputStream(filename)));
            Codec codec = new Codec();

            while (true) {
                SeedRecord sr = SeedRecord.read(dis, 4096);

                // maybe print it out...
                // sr.writeASCII(out);

                int numSoFar = 0;
                if (sr instanceof DataRecord) {
                    DataRecord dr = (DataRecord) sr;
                    Blockette1000 b1000 = (Blockette1000) dr.getBlockettes(1000)[0];

                    System.out.print("\n\nStart time: " + dr.getHeader().getStartTime());
                    // System.out.print("\tData record lenght: " + b1000.getDataRecordLength());
                    System.out.print("\tSequence Number: " + dr.getHeader().getSequenceNum());
                    System.out.print("\tID: " + dr.getHeader().getStationIdentifier());
                    System.out.print("\tChannel: " + dr.getHeader().getChannelIdentifier());
                    System.out.println("\tLocation: " + dr.getHeader().getLocationIdentifier());
                    System.out.print("Number Samples: " + dr.getHeader().getNumSamples());
                    System.out.println("\tSPS: " + dr.getHeader().getSampleRateFactor());

                    float[] data = new float[dr.getHeader().getNumSamples()];

                    DecompressedData decompData = codec.decompress(b1000
                            .getEncodingFormat(), dr.getData(), dr.getHeader()
                            .getNumSamples(), b1000.isLittleEndian());
                    float[] temp = decompData.getAsFloat();
                    System.arraycopy(temp, 0, data, numSoFar, temp.length);

                    int i = 0;
                    for(float f : data){
                        i += 1;
                        System.out.print(String.format("%05d\t", Math.round(f)));
                        if(i >= 24){
                            i=0;
                            System.out.println("");
                        }
                    }

                    numSoFar += temp.length;
                }
            }
        } catch (EOFException e) {
            System.out.println("EOF Exception");
            e.printStackTrace();
        } catch (FileNotFoundException e) {
            System.out.println("File not found Exception");
            e.printStackTrace();
        } catch (IOException e) {
            System.out.println("IO exception");
            e.printStackTrace();
        } catch (SeedFormatException e) {
            System.out.println("Seed formater");
            e.printStackTrace();
        } catch (UnsupportedCompressionType e) {
            System.out.println("UnsupportedCompressionType");
            e.printStackTrace();
        } catch (CodecException e) {
            System.out.println("CodecException");
            e.printStackTrace();
        }
    }
}
