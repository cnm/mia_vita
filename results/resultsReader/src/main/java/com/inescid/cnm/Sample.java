package com.inescid.cnm;

import java.text.SimpleDateFormat;
import java.util.Comparator;
import java.util.Date;

public class Sample {
    private final Date ts; 
    private final float v;
    private static SimpleDateFormat df = new SimpleDateFormat("yyyy,DDD,HH:mm:ss.SSS");

    public Sample(Date ts, float v)
    {
        this.ts = ts;
        this.v = v;
    }

    @Override
    public String toString() {
        return String.format("%s:\t%f", df.format(ts), v);
    };


    public String toString(boolean withTime) {
        if(withTime){
            return this.toString();
        }
        else
        {
            return String.format("%f", ts);
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

    public static class SampleComparator implements Comparator<Sample>{
        public int compare(Sample s1, Sample s2) {
            return s1.ts.compareTo(s2.ts);
        }
    }
}
