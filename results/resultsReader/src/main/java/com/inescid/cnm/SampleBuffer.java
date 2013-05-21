package com.inescid.cnm;

import org.apache.commons.collections.buffer.CircularFifoBuffer;

public class SampleBuffer {

    private final CircularFifoBuffer buff; 
    private float sums;

    public SampleBuffer(int size){
        buff = new CircularFifoBuffer(size);
        sums = 0;
    }

    public void add(Sample new_sample){
        // If it is the beginning
        if (buff.maxSize() > buff.size())
        {
            //Do nothing
        }
            
        // If we already have full buffer
        else 
        {
            sums -= Math.abs(((Sample) buff.get()).getValue());
        }

        buff.add(new_sample);

        sums += Math.abs(new_sample.getValue());
    }

    public float getSum()
    {
        return sums;
    }

    public boolean isFull()
    {
        return buff.isFull();
    }

    public Sample get()
    {
        return (Sample) buff.get();
    }
}
