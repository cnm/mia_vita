package com.inescid.cnm;

import java.text.SimpleDateFormat;
import java.util.Comparator;
import java.util.Date;

public class Sample {
    private static int numberSamples = 0;

    private final Date ts; 
    private final float v;
    private final int sn;

    // private static SimpleDateFormat df = new SimpleDateFormat("yyyy-MM-dd_HH:mm:ss.SSS");
    private static SimpleDateFormat df = new SimpleDateFormat("HHmmssSSS");

    public Sample(Date ts, float v)
    {
        this.ts = ts;
        this.v = v;
        this.sn = numberSamples++;
    }

    @Override
    public String toString() {
        // return String.format("%s\t%f", df.format(ts), v); // This shows a pretty date
        return String.format("%s\t%f", ts.getTime(), v); // This shows the time in miliseconds since 1970
    };


    public String toString(boolean withTime) {
        if(withTime){
            return this.toString();
        }
        else
        {
            return String.format("%d\t%f", sn, v);
        }
    };

    @Override
    public boolean equals(Object obj) {
        return this.ts.equals(obj);
    };

    @Override
    public int hashCode() {
        return this.ts.hashCode();
    };

    public static class SampleComparatorTime implements Comparator<Sample>{
        public int compare(Sample s1, Sample s2) {
            return s1.ts.compareTo(s2.ts);
        }
    }

    public static class SampleComparatorSequenceNumber implements Comparator<Sample>{
        public int compare(Sample s1, Sample s2) {
            return Double.compare(s1.sn, s2.sn);
        }
    }
}
