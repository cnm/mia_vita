package com.inescid.cnm;

import java.io.BufferedInputStream;
import java.io.BufferedWriter;
import java.io.DataInput;
import java.io.DataInputStream;
import java.io.EOFException;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileWriter;
import java.io.IOException;
import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Calendar;
import java.util.Date;
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

public class App
{
    public static void main(String[] args)
    {
        TreeMap<DataRecord, float[]> orderedRecordDataMap;
        ReaderCliOptions opt = new ReaderCliOptions();
        opt.parse(args);
        String inputMseedPath = opt.mseedPath;
        System.out.println(opt + "\n");

        try
        {
            System.out.println(String.format("Reading mseed file: %s", opt.mseedPath));
            orderedRecordDataMap = decompressDataRecordList(getAllDataRecords(inputMseedPath));

            System.out.println("Writting to output file treated data");
            writeOrderedRecordDataMap(orderedRecordDataMap, opt.softLineLimit, opt.softLineLimitValue, opt.outputDataFilePath);
        }
        catch (FileNotFoundException e1)
        {
            System.out.println("Unable to find file: " + inputMseedPath);
            System.exit(1);
        }
    }

    private static void writeOrderedRecordDataMap(TreeMap<DataRecord, float[]> orderedRecordDataMap, Boolean softLineLimit, int softLineLimitValue, String dataOutFilepath)
    {
        BufferedWriter out;
        int lines = 0;

        try
        {
            out = new BufferedWriter(new FileWriter(dataOutFilepath));

            for (float[] v : orderedRecordDataMap.values())
            {
                for (float f : v)
                {
                    lines +=1;
                    out.write(Float.toString(f) + '\n');
                }

                if(softLineLimit && lines > softLineLimitValue)
                {
                    break;   
                }
            }

            System.out.println(String.format("Written: %d lines to file: %s", lines, dataOutFilepath));
            out.close();
        }
        catch (IOException e)
        {
            System.out.println("Could not write data file");
            e.printStackTrace();
        }
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

    private static TreeMap<DataRecord, float[]> decompressDataRecordList(List<DataRecord> records)
    {
        TreeMap<DataRecord, float[]> orderedRecordDataMap = new TreeMap<DataRecord, float[]>(new DataRecordBeginComparator());

        for (DataRecord record : records)
        {
            orderedRecordDataMap.put(record, decompressDataRecord(record));
        }
        return orderedRecordDataMap;
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
                    records.add(dr);
                    // printHeader(dr);
                }
            }
            catch (EOFException e)
            {
                System.out.println("EOF Exception --> Indicate the input has no more to read. Not an error");
                eofReached = true; // To get out of the loop
            }
            catch (IOException e)
            {
                System.out.println("IO exception");
                e.printStackTrace();
                System.exit(1);
            }
            catch (SeedFormatException e)
            {
                System.out.println("Exception with the mseed format");
                e.printStackTrace();
                System.exit(1);
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
        System.out.println("\n");
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

    private static void printHeader(DataRecord dr)
    {
        // Date to format ---> 2013,038,01:22:05.0000
        SimpleDateFormat df = new SimpleDateFormat("yyyy,DDD,HH:mm:ss.SSS");

        DataHeader header = dr.getHeader();
        Calendar cal = Calendar.getInstance();   

        try
        {
            Date start = df.parse(header.getStartTime().substring(0, header.getStartTime().length() - 1));  // Removed last character as it is always 0
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
            System.exit(1);
        }
    }
}
