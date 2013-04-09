package com.inescid.cnm;

import java.text.SimpleDateFormat;
import java.util.Comparator;
import java.util.Date;

public class Sample {
    private static int numberSamples = 0;

    private final Date ts; 
    private final float v;
    private final int sn;

    private static SimpleDateFormat df = new SimpleDateFormat("yyyyMMddHHmmssSSS");

    public Sample(Date ts, float v)
    {
        this.ts = new Date(ts.getTime());
        this.v = v;
        this.sn = numberSamples++;
    }

    public static void changeDataFormat(String dateFormat){
        df = new SimpleDateFormat(dateFormat);
    }

    @Override
    public boolean equals(Object obj) {
        return this.ts.equals(obj);
    };

    @Override
    public int hashCode() {
        return this.ts.hashCode();
    };

    @Override
    public String toString() {
        return toStringDate();
    };

    public String toStringDate() {
        return String.format("%s\t%f", df.format(ts), v); // This shows a pretty date
    };

    public String toString(boolean withTime) {
        if(withTime)
        {
            return toStringDate();
        }
        else
        {
            return toStringTimeStamp();
        }
    };

    public String toStringTimeStamp() {
        return String.format("%d\t%f", sn, v);
    }

    public String toStringTimeEpoch() {
        return String.format("%s\t%f", ts.getTime(), v); // This shows the time in miliseconds since 1970
    }

    public static class SampleComparatorTime implements Comparator<Sample> {
        public int compare(Sample s1, Sample s2) {
            return s1.ts.compareTo(s2.ts);
        }
    }

    public static class SampleComparatorSequenceNumber implements Comparator<Sample> {
        public int compare(Sample s1, Sample s2) {
            return Double.compare(s1.sn, s2.sn);
        }
    }
}
