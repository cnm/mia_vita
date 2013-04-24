package com.inescid.cnm;

import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Collection;
import java.util.Iterator;

import org.json.simple.JSONArray;
import org.json.simple.JSONObject;
import org.json.simple.parser.JSONParser;
import org.json.simple.parser.ParseException;

import edu.sc.seis.seisFile.mseed.DataRecord;

public class ReadOur implements IDataReader
{
    JSONParser parser = new JSONParser();

    @Override
    public Collection<DataRecord> getAllDataRecords(String filename) throws FileNotFoundException
    {
        Collection<DataRecord> records = new ArrayList<DataRecord>(); // List which will store the data records

        Object obj;
        try
        {
            obj = parser.parse(new FileReader(filename));

            JSONArray jsonArray = (JSONArray) obj;

            Iterator<String> iterator = jsonArray.iterator();
            while (iterator.hasNext()) {
                System.out.println(iterator.next());
            }
        }

        catch (IOException e)
        {
            throw new RuntimeException(e);
        }
        catch (ParseException e)
        {
            throw new RuntimeException(e);
        }

        return records;
    }

    @Override
    public float getSPS()
    {
        // TODO Auto-generated method stub
        return 0;
    }

}
