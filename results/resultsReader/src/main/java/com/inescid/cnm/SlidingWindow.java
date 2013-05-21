package com.inescid.cnm;

public class SlidingWindow {
    final SampleBuffer before;
    final SampleBuffer after;
    Sample middle;

    public SlidingWindow(int size)
    {
        before = new SampleBuffer(size);
        after = new SampleBuffer(size);
        middle = null;
    }

    public void add(Sample s)
    {
        if(!before.isFull())
        {
            before.add(s);
        }

        else if(middle == null)
        {
            middle = s;
        }

        else if (!after.isFull())
        {
            after.add(s);
        }

        // Everything is full (normal case)
        else
        {
            before.add(middle);
            middle = after.get();
            after.add(s);
        }
    }

    public boolean isMiddleOutlier()
    {
        if(middle == null)
            return false;

        if(before.getSum() < Math.abs(middle.getValue()) * 2 && after.getSum() < Math.abs(middle.getValue()) * 2)
        {
            return true;
        }
        return false;
    }

    public Sample getMiddle()
    {
        return middle;
    }
}
