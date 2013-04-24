package com.inescid.cnm;

import java.io.FileNotFoundException;
import java.util.Collection;

import edu.sc.seis.seisFile.mseed.DataRecord;

interface IDataReader {
    public Collection<DataRecord> getAllDataRecords(String filename) throws FileNotFoundException;
    public float getSPS();
}
