package com.inescid.cnm;

import java.io.BufferedReader;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.IOException;
import java.math.BigDecimal;
import java.util.Collections;
import java.util.Date;
import java.util.LinkedList;
import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class ReadJson implements IDataReader
{
    private float        SPS;
    private List<Sample> sampleList;
    private final int channel;

    public ReadJson(String filename, int channel) throws FileNotFoundException
    {
        this.channel = channel;
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
                    float value           = Float.parseFloat(m.group(channel + 3));

                    // timestamp string fetched (space not included) ---> 1360195908 608085 --> (seconds microseconds)
                    BigDecimal timestampOnlyMiliSeconds = timestamp.divide(new BigDecimal(1000));
                    long milisecondsInOneHour = 1000*60*60;
                    Date date = new Date(timestampOnlyMiliSeconds.longValue() - milisecondsInOneHour);
                    Sample sample = new Sample(date, value);
                    sampleList.add(sample);
                }
            }
            br.close();

            // Let's make an assumption of the SPS
            Date before = sampleList.get(0).getTs();
            Date next   = sampleList.get(1).getTs();
            this.SPS = new Float(1000) / (next.getTime() - before.getTime());

            Collections.sort(sampleList, new Sample.SampleComparatorTime());

            return sampleList;
        }
        catch (IOException e)
        {
            e.printStackTrace();
            throw new FileNotFoundException();
        }
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
