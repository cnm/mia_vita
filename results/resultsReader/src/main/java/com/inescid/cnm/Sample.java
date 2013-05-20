package com.inescid.cnm;

import java.text.SimpleDateFormat;
import java.util.Comparator;
import java.util.Date;

public class Sample {
    private static int numberSamples = 0;

    private final Date ts; 
    private final float v;
    private final int sn;

    private static SimpleDateFormat df = new SimpleDateFormat("yyyy_MM_dd_HH:mm:ss.SSS");

    public Sample(Date ts, float v)
    {
        this.ts = new Date(ts.getTime());
        this.v = v;
        this.sn = numberSamples++;
    }

    public static void changeDateFormat(String dateFormat){
        df = new SimpleDateFormat(dateFormat);
    }

    @Override
    public boolean equals(Object other) {
        if (this == other) {
            return true;
        }
        if (other == null) {
            return false;
        }
        if (getClass() != other.getClass()) {
            return false;
        }
        Sample otherSample = (Sample) other;
        if (this.getTs() == null) {
            if (otherSample.getTs() != null) {
                return false;
            }
        } else if (!this.getTs().equals(otherSample.getTs())) {
            return false;
        }
        return true;
    };

    @Override
    public int hashCode() {
        return this.ts.hashCode();
    };

    @Override
    public String toString() {
        return toStringDate();
    };

    public String toStringJustTS() {
        return String.format("%s", df.format(ts)); // This shows only the timestamp
    };

    public String toStringDate() {
        return String.format("%s\t%f", df.format(ts), v); // This shows a pretty date
    };

    public String toString(boolean withTime) {
        if(withTime)
        {
            return toStringTimeEpoch();
        }
        else
        {
            return toStringDate();
        }
    };


    public Date getTs()
    {
        return ts;
    }

    public float getValue()
    {
        return v;
    }

    public String toStringSequenceNumber() {
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
