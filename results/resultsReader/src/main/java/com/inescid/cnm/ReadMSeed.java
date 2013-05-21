package com.inescid.cnm;

import java.io.BufferedInputStream;
import java.io.DataInput;
import java.io.DataInputStream;
import java.io.EOFException;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Calendar;
import java.util.Collection;
import java.util.Collections;
import java.util.Date;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;
import java.util.TreeMap;

import edu.iris.dmc.seedcodec.Codec;
import edu.iris.dmc.seedcodec.CodecException;
import edu.iris.dmc.seedcodec.DecompressedData;
import edu.iris.dmc.seedcodec.UnsupportedCompressionType;
import edu.sc.seis.seisFile.mseed.Blockette1000;
import edu.sc.seis.seisFile.mseed.DataHeader;
import edu.sc.seis.seisFile.mseed.DataRecord;
import edu.sc.seis.seisFile.mseed.DataRecordBeginComparator;
import edu.sc.seis.seisFile.mseed.SeedFormatException;
import edu.sc.seis.seisFile.mseed.SeedRecord;

public class ReadMSeed implements IDataReader
{

    private float SPS;
    List<Sample>  sampleList;

    public ReadMSeed(String filename, boolean debug, boolean inputWithSequenceNumber) throws FileNotFoundException
    {
        TreeMap<DataRecord, float[]> orderedRecordDataMap;
        orderedRecordDataMap = decompressDataRecordList(getAllDataRecords(filename));

        if (debug)
        {
            printOrderedRecordDataMap(orderedRecordDataMap);
        }

        sampleList = transformToOrderedSampleList(orderedRecordDataMap, inputWithSequenceNumber);
    }

    private static TreeMap<DataRecord, float[]> decompressDataRecordList(Collection<DataRecord> records)
    {
        TreeMap<DataRecord, float[]> orderedRecordDataMap = new TreeMap<DataRecord, float[]>(new DataRecordBeginComparator());

        for (DataRecord record : records)
        {
            orderedRecordDataMap.put(record, decompressDataRecord(record));
        }
        return orderedRecordDataMap;
    }

    private static float[] decompressDataRecord(DataRecord dr)
    {
        Codec codec = new Codec();

        assert dr.getBlockettes().length == 1;
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

    public Collection<DataRecord> getAllDataRecords(String filename) throws FileNotFoundException
    {
        DataInput dis;
        Boolean eofReached = false;

        dis = new DataInputStream(new BufferedInputStream(new FileInputStream(filename)));

        Collection<DataRecord> records = new ArrayList<DataRecord>(); // List which will store the data records

        // Continue reading until EOF given by exception
        while (!eofReached)
        {
            try
            {
                SeedRecord sr = SeedRecord.read(dis, 4096); // 4096 is for read miniseed that lack a Blockette1000

                // We should only find data records
                assert sr instanceof DataRecord;
                DataRecord dr = (DataRecord) sr;
                records.add(dr);

                setSPS(dr.getHeader().getSampleRate());
            }
            catch (EOFException e)
            {
                System.out.println("EOF Exception --> Indicate the input has no more to read. Not an error");
                eofReached = true; // To get out of the loop
            }
            catch (IOException e)
            {
                System.out.println("IO exception. Quitting");
                e.printStackTrace();
                System.exit(1);
            }
            catch (SeedFormatException e)
            {
                System.out.println("Exception with the mseed format. Quitting");
                e.printStackTrace();
                System.exit(1);
            }
        }// While

        return records;
    }

    private void setSPS(float sampleRate)
    {
        this.SPS = sampleRate;
    }

    private static void printHeader(DataRecord dr)
    {
        // Date to format ---> 2013,038,01:22:05.0000
        SimpleDateFormat df = new SimpleDateFormat("yyyy,DDD,HH:mm:ss.SSS");

        DataHeader header = dr.getHeader();
        Calendar cal = Calendar.getInstance();

        Date start;
        try
        {
            start = df.parse(header.getStartTime().substring(0, header.getStartTime().length() - 1)); // Removed last character as it is always 0
            int timeInRecord = (1000 / header.getSampleRateFactor()) * header.getNumSamples();
            cal.setTime(start);
            cal.add(Calendar.MILLISECOND, timeInRecord);

            System.out.print("Start: " + df.format(start));
            System.out.print("\tEnd: " + df.format(cal.getTime()));
            System.out.print("\tSequence Number: " + header.getSequenceNum());
            System.out.print("\tID: " + header.getStationIdentifier());
            System.out.print("\tChannel: " + header.getChannelIdentifier());
            System.out.print("\tLocation: " + header.getLocationIdentifier());
            System.out.print("\tNumber Samples: " + header.getNumSamples());
            System.out.print("\tSPS: " + header.getSampleRateFactor());
            System.out.println(" x " + header.getSampleRateMultiplier());
        }
        catch (ParseException e)
        {
            e.printStackTrace();
        } 
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
        System.out.println("\n");
    }

    private static void printOrderedRecordDataMap(TreeMap<DataRecord, float[]> orderedRecordDataMap)
    {
        for (Map.Entry<DataRecord, float[]> entry : orderedRecordDataMap.entrySet())
        {
            DataRecord dr = entry.getKey();
            float[] data = entry.getValue();

            printHeader(dr);
            printResults(data);
        }
    }


    private static List<Sample> transformToOrderedSampleList(final TreeMap<DataRecord, float[]> orderedRecordDataMap, Boolean inputWithSequenceNumber)
    {
        Calendar cal = Calendar.getInstance();   
        ArrayList<Sample> sampleList = new ArrayList<Sample>();

        try
        {
            for (Map.Entry<DataRecord, float[]> entry : orderedRecordDataMap.entrySet())
            {

                int i = 0;
                float[] values = entry.getValue();
                DataRecord dr = entry.getKey();

                SimpleDateFormat df = new SimpleDateFormat("yyyy,DDD,HH:mm:ss.SSS");
                DataHeader header = dr.getHeader();

                Date start = df.parse(header.getStartTime().substring(0, header.getStartTime().length() - 1));  // Removed last character as it is always 0

                for (float f : values)
                {

                    int timeInRecord = (1000 / header.getSampleRateFactor()) * i;
                    i += 1;
                    cal.setTime(start);
                    cal.add(Calendar.MILLISECOND, timeInRecord);

                    sampleList.add(new Sample(cal.getTime(), f));
                }
            }

        }
        catch (ParseException e)
        {
            System.out.println("Parse exception");
            e.printStackTrace();
            System.out.println("Continuing");
        }

        System.out.println("Sorting...");

        if(inputWithSequenceNumber)
            Collections.sort(sampleList, new Sample.SampleComparatorSequenceNumber());
        else
            Collections.sort(sampleList, new Sample.SampleComparatorTime());

        return new LinkedList<Sample>(sampleList);
    }



    public float getSPS()
    {
        return SPS;
    }

    public List<Sample> getSamples()
    {
        return sampleList;
    }
}
