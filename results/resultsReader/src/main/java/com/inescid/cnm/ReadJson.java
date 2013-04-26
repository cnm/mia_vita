package com.inescid.cnm;

import java.io.FileReader;
import java.io.IOException;
import java.util.List;

import org.json.simple.JSONArray;
import org.json.simple.parser.JSONParser;
import org.json.simple.parser.ParseException;

public class ReadJson implements IDataReader
{
    private float        SPS;
    private List<Sample> sampleList;

    public ReadJson(String filename)
    {
        sampleList = getAllSamples(filename);
    }

    private List<Sample> getAllSamples(String filename)
    {
        JSONParser parser = new JSONParser();

        try
        {
            Object obj = parser.parse(new FileReader(filename));
            JSONArray array=(JSONArray)obj;
            System.out.println(array);
            System.out.println(array.get(0));
        }
        catch (IOException e)
        {
            e.printStackTrace();
        }
        catch (ParseException e)
        {
            e.printStackTrace();
        }

        return null;
    }

    public float getSPS()
    {
        return this.SPS;
    }

    public List<Sample> getSamples()
    {
        return sampleList;
    }
}
