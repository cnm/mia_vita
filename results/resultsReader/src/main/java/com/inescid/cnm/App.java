package com.inescid.cnm;

import java.io.BufferedInputStream;
import java.io.DataInput;
import java.io.DataInputStream;
import java.io.EOFException;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

import edu.iris.dmc.seedcodec.Codec;
import edu.iris.dmc.seedcodec.CodecException;
import edu.iris.dmc.seedcodec.DecompressedData;
import edu.iris.dmc.seedcodec.UnsupportedCompressionType;
import edu.sc.seis.seisFile.mseed.Blockette1000;
import edu.sc.seis.seisFile.mseed.DataRecord;
import edu.sc.seis.seisFile.mseed.SeedFormatException;
import edu.sc.seis.seisFile.mseed.SeedRecord;

public class App
{
    public static final String DEFAULT_FILENAME = "../Sesimbra_07_Jan_2012/theirs/IP.PSES..BHN.D.2013.038";

    public static void main(String[] args)
    {
        List<DataRecord> records = new ArrayList<DataRecord>(); // List which will store the data records
        String filename = DEFAULT_FILENAME;

        try
        {
            records = getAllDataRecords(filename);
        }
        catch (FileNotFoundException e1)
        {
            System.out.println("Unable to find file: " + filename);
            System.exit(1);
        }
    }

    private static List<DataRecord> getAllDataRecords(String filename) throws FileNotFoundException
    {
        DataInput dis;
        Boolean eofReached = false;

        dis = new DataInputStream(new BufferedInputStream(new FileInputStream(filename)));

        List<DataRecord> records = new ArrayList<DataRecord>(); // List which will store the data records 
        while (!eofReached)
        {
            try
            {
                SeedRecord sr = SeedRecord.read(dis, 4096); // 4096 is for read miniseed that lack a Blockette1000

                if (sr instanceof DataRecord)
                {
                    DataRecord dr = (DataRecord) sr;
                    // records.add(dr);
                    printHeader(dr);

                    float[] data = decompressDataRecord(dr);
                    printResults(data);

                }
            }
            catch (EOFException e)
            {
                System.out.println("EOF Exception");
                eofReached = true; // To get out of the loop
            }
            catch (IOException e)
            {
                System.out.println("IO exception");
                e.printStackTrace();
            }
            catch (SeedFormatException e)
            {
                System.out.println("Seed formater");
                e.printStackTrace();
            }
        }// While
        return records;
    }

    private static void printResults(float[] data)
    {
        int i = 0;
        for (float f : data)
        {
            i += 1;
            System.out.print(String.format("%05d\t", Math.round(f)));
            if (i >= 24)
            {
                i = 0;
                System.out.println("");
            }
        }
    }

    private static float[] decompressDataRecord(DataRecord dr)
    {
        Codec codec = new Codec();
        Blockette1000 b1000 = (Blockette1000) dr.getBlockettes(1000)[0];
        float[] data = new float[dr.getHeader().getNumSamples()];

        DecompressedData decompData;
        try
        {
            decompData = codec.decompress(b1000.getEncodingFormat(), dr.getData(), dr.getHeader().getNumSamples(), b1000.isLittleEndian());

            float[] temp = decompData.getAsFloat();
            int numSoFar = 0;

            System.arraycopy(temp, 0, data, numSoFar, temp.length);
            numSoFar += temp.length;
        }
        catch (UnsupportedCompressionType e)
        {
            System.out.println("UnsupportedCompressionType");
            e.printStackTrace();
        }
        catch (CodecException e)
        {
            System.out.println("CodecException");
            e.printStackTrace();
        }

        return data;
    }

    private static void printHeader(DataRecord dr)
    {
        System.out.print("\n\nStart time: " + dr.getHeader().getStartTime());
        // System.out.print("\tData record lenght: " +
        // b1000.getDataRecordLength());
        System.out.print("\tSequence Number: " + dr.getHeader().getSequenceNum());
        System.out.print("\tID: " + dr.getHeader().getStationIdentifier());
        System.out.print("\tChannel: " + dr.getHeader().getChannelIdentifier());
        System.out.println("\tLocation: " + dr.getHeader().getLocationIdentifier());
        System.out.print("Number Samples: " + dr.getHeader().getNumSamples());
        System.out.println("\tSPS: " + dr.getHeader().getSampleRateFactor());
    }
}
