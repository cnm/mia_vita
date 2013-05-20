package com.inescid.cnm;

import java.io.BufferedWriter;
import java.io.FileNotFoundException;
import java.io.FileWriter;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Collection;
import java.util.List;

public class App
{
    public static float SPS;
    public static int numberOverlaps = 0;
    public static int numberGaps = 0;

    public static void main(String[] args)
    {
        ReaderCliOptions opt = new ReaderCliOptions();
        opt.parse(args);
        System.out.println(opt + "\n");

        //Lets read the mseed file, order it, transform it to a collection of samples, and then write the samples in a file
        try
        {
            System.out.println(String.format("Reading mseed file: %s", opt.inputFilePath));

            IDataReader reader;

            // Prepare the reader for the input files
            if(opt.isInputJson)
                reader = new ReadJson(opt.inputFilePath, opt.channel);
            else
                reader = new ReadMSeed(opt.inputFilePath, opt.debug, opt.inputWithSequenceNumber);

            // Let's set the SPS
            if(opt.forcedSPS)
            {
                SPS = opt.SPS;
            }
            else
            {
                SPS = reader.getSPS();
                System.out.println("Assuming SPS is " + SPS);
            }

            // Lets finally read the input files
            List<Sample> sampleList = reader.getSamples();

            // Validate the obtained samples
            Collection<Sample> outlierList = new ArrayList<Sample>();
            if(validSampleList(sampleList, outlierList))
            {
                System.out.println("All samples are valid");
            }

            // Print the stats
            printStatistics(sampleList.size(), numberOverlaps, numberGaps);

            // Write the stats orderly to csv file
            if (!opt.onlyCheck)
            {
                System.out.println("Writting to output file treated data");
                writeSampleList(sampleList, outlierList, opt.outputDataFilePath, opt.softLineLimit, opt.softLineLimitValue, opt.outputWithTimeSinceEpoch);
            }
        }
        catch (FileNotFoundException e1)
        {
            System.out.println("Unable to find input mseed file: " + opt.inputFilePath);
            System.exit(1);
        }
    }

    private static boolean validSampleList(List<Sample> sampleList, Collection<Sample> outlierList)
    {
        long delta = 0;
        Sample last_sample = sampleList.get(0);
        long last_sample_time = sampleList.get(0).getTs().getTime();
        boolean valid = true;
        boolean first = true;

        // let's give a 15 miliseconds error margin to not consider there is a gap (sample with sligh delay)
        int error = 15;

        SlidingWindow window = new SlidingWindow(10);

        // Lets traverse all samples
        for (Sample s : sampleList)
        {
            long this_sample_time = s.getTs().getTime();
            delta = this_sample_time - last_sample_time;

            // Check for gaps in time collection
            // If delta is higher than the SPS
            if (delta > ((1000 / SPS) + error) && !first)
            {
                System.out.println("Expected Delta/ True Delta: " + (1000 / SPS) + " / " + delta + 
                        "\t\tGap in data records at time: " + s.toStringJustTS() + " / " + last_sample.toStringJustTS());
                System.out.println(s.getTs().getTime());
                numberGaps += 1;
                valid = false;                  
            }

            // Check if they are repeated values
            if (this_sample_time == last_sample_time && !first)
            {
                System.out.println("Repeated sample with time: " + s.toStringDate() + "\t\t" + last_sample.toStringDate());
                numberOverlaps += 1;
                valid = false;                  
            }

            last_sample = s;
            last_sample_time = this_sample_time;
            first = false;


            // Lets check if it is an outlier
            window.add(s);
            if(window.isMiddleOutlier()){
                System.out.println("I thing is outlier: " + window.getMiddle());
                outlierList.add(window.getMiddle());
            }
        }

        sampleList.removeAll(outlierList);

        for (Sample out : outlierList)
            sampleList.add(new Sample(out.getTs(), 9999999));

        return valid;
    }

    private static void writeSampleList(Collection<Sample> sampleList, Collection<Sample> outlierList, String dataOutFilepath, Boolean softLineLimit, 
            int softLineLimitValue, Boolean outputWithTimeSinceEpoch){
        BufferedWriter out;
        int lines = 0;
        try
        {
            out = new BufferedWriter(new FileWriter(dataOutFilepath));
            for(Sample sample : sampleList){
                out.write(sample.toString(outputWithTimeSinceEpoch) + "\n");

                lines +=1;
                if(softLineLimit && lines > softLineLimitValue)
                {
                    break;   
                }
            }

            out.close();
        }

        catch (IOException e)
        {
            System.out.println("Could not write data file");
            e.printStackTrace();
        }
    }

    private static void printStatistics(int number_samples, int numberOverlaps, int numberGaps)
    {
        float n_samp = (float) number_samples;
        String message = String.format("Analysed %f samples. Overlaps: %d (%f%%)\tGaps: %d (%f%%)", 
                n_samp, numberOverlaps, (numberOverlaps / n_samp) * 100, numberGaps, (numberGaps / n_samp) * 100);
        System.out.println(message);
    }
}
