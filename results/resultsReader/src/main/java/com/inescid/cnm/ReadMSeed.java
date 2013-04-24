package com.inescid.cnm;

import java.io.BufferedInputStream;
import java.io.DataInput;
import java.io.DataInputStream;
import java.io.EOFException;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Collection;

import edu.sc.seis.seisFile.mseed.DataRecord;
import edu.sc.seis.seisFile.mseed.SeedFormatException;
import edu.sc.seis.seisFile.mseed.SeedRecord;

public class ReadMSeed implements IDataReader {
    private float SPS;

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

    public float getSPS()
    {
        return SPS;
    }
}
