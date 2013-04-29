package com.inescid.cnm;

import java.io.BufferedReader;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.IOException;
import java.math.BigDecimal;
import java.util.Date;
import java.util.LinkedList;
import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class ReadJson implements IDataReader
{
    private float        SPS;
    private List<Sample> sampleList;

    public ReadJson(String filename) throws FileNotFoundException
    {
        sampleList = getAllSamples(filename);
    }

    private List<Sample> getAllSamples(String filename) throws FileNotFoundException
    {
        List<Sample> sampleList = new LinkedList<Sample>();

        BufferedReader br = new BufferedReader(new FileReader(filename));
        String line;
        try
        {
            while ((line = br.readLine()) != null)
            {
                if (line.length() > 4){

                // "1:37856":{"ts":1360197059933431,"1":-2999,"2":90622,"3":-103697,"4": 4788355},

                    Pattern p = Pattern.compile("\"(\\d+):(\\d+)\":\\{\"ts\":(\\d+),\"1\":(-?\\d+),\"2\":(-?\\d+),\"3\":(-?\\d+),\"4\":\\ (-?\\d+)\\},?");
                    Matcher m = p.matcher(line);

                    m.groupCount();
                    m.find();

                    BigDecimal timestamp  = new BigDecimal(m.group(3));
                    float valueX     = Float.parseFloat(m.group(4));
                    float valueY     = Float.parseFloat(m.group(5));
                    float valueZ     = Float.parseFloat(m.group(6));
                    float bat0       = Float.parseFloat(m.group(7));

                    // timestamp string fetched (space not included) ---> 1360195908 608085 --> (seconds microseconds)
                    BigDecimal timestampOnlyMiliSeconds = timestamp.divide(new BigDecimal(1000));
                    Date date = new Date(timestampOnlyMiliSeconds.longValue());
                    Sample sample = new Sample(date, valueX);
                    sampleList.add(sample);
                }
            }
            br.close();
            return sampleList;
        }
        catch (IOException e)
        {
            e.printStackTrace();
            throw new FileNotFoundException();
        }
    }



    // private List<Sample> getAllSamples(String filename)
    // {
    //     JSONParser parser = new JSONParser();

    //     try
    //     {
    //         Object obj = parser.parse(new FileReader(filename));
    //         JSONArray array=(JSONArray)obj;
    //         System.out.println(array);
    //         System.out.println(array.get(0));
    //     }
    //     catch (IOException e)
    //     {
    //         e.printStackTrace();
    //     }
    //     catch (ParseException e)
    //     {
    //         e.printStackTrace();
    //     }

    //     return null;
    // }

    public float getSPS()
    {
        return this.SPS;
    }

    public List<Sample> getSamples()
    {
        return sampleList;
    }
}
