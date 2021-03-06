package com.inescid.cnm;

import org.apache.commons.cli.CommandLine;
import org.apache.commons.cli.CommandLineParser;
import org.apache.commons.cli.HelpFormatter;
import org.apache.commons.cli.OptionBuilder;
import org.apache.commons.cli.Options;
import org.apache.commons.cli.ParseException;
import org.apache.commons.cli.PosixParser;

public class ReaderCliOptions extends Options
{
    private static final String DEFAULT_INPUT_MSEED_PATH = "../Sesimbra_07_Jan_2012/theirs/IP.PSES..BHN.D.2013.038";

    private static final long serialVersionUID = 1L;

    public String inputFilePath = DEFAULT_INPUT_MSEED_PATH;
    public String outputDataFilePath = "out.data";
    public Boolean softLineLimit = false;
    public int softLineLimitValue = -1;
    public boolean debug = false;
    public boolean inputWithSequenceNumber = false;
    public boolean outputWithTimeSinceEpoch = false;
    public boolean onlyCheck = false;
    public boolean isInputJson = false;
    public float SPS = 0;
    public boolean forcedSPS = false;
    public int channel = 1;

    public ReaderCliOptions()
    {
        super();

        OptionBuilder.withLongOpt("write-limit");
        OptionBuilder.withDescription("Limits the number of samples to write");
        OptionBuilder.withType(Number.class);
        OptionBuilder.hasArg();
        OptionBuilder.withArgName("number");
        this.addOption(OptionBuilder.create());

        OptionBuilder.withLongOpt("input");
        OptionBuilder.withDescription("Input filepath");
        OptionBuilder.withType(String.class);
        OptionBuilder.hasArg();
        OptionBuilder.withArgName("input-file-path");
        this.addOption(OptionBuilder.create());

        OptionBuilder.withLongOpt("data-output");
        OptionBuilder.withDescription("Filepath to write the data output");
        OptionBuilder.withType(String.class);
        OptionBuilder.hasArg();
        OptionBuilder.withArgName("output-path");
        this.addOption(OptionBuilder.create());

        OptionBuilder.withLongOpt("SPS");
        OptionBuilder.withDescription("Force a sample per second (SPS) value");
        OptionBuilder.withType(Number.class);
        OptionBuilder.hasArg();
        OptionBuilder.withArgName("sps-value");
        this.addOption(OptionBuilder.create());

        OptionBuilder.withLongOpt("channel");
        OptionBuilder.withDescription("Force the JSON to use a specific channel (default 1)");
        OptionBuilder.withType(Number.class);
        OptionBuilder.hasArg();
        OptionBuilder.withArgName("channel number");
        this.addOption(OptionBuilder.create());

        OptionBuilder.withLongOpt("help");
        OptionBuilder.withDescription("print this message and exit");
        this.addOption(OptionBuilder.create());

        OptionBuilder.withLongOpt("debug");
        OptionBuilder.withDescription("Prints mseed info as it reads the input file");
        this.addOption(OptionBuilder.create());

        OptionBuilder.withLongOpt("only-check");
        OptionBuilder.withDescription("Only checks input file and does not write output file");
        this.addOption(OptionBuilder.create());

        OptionBuilder.withLongOpt("outputWithTimeSinceEpoch");
        OptionBuilder.withDescription("Is the output to be written dates in unix time (with miliseconds) - Default print a human readable date");
        this.addOption(OptionBuilder.create());

        OptionBuilder.withLongOpt("inputWithSequenceNumber");
        OptionBuilder.withDescription("Input has sequence number instead of timestamp");
        this.addOption(OptionBuilder.create());

        OptionBuilder.withLongOpt("isInputJson");
        OptionBuilder.withDescription("Is Input in json format");
        this.addOption(OptionBuilder.create());
    }

    void parse(String[] args)
    {
        CommandLineParser cmdLineParser = new PosixParser();

        try {
            CommandLine cmdLine = cmdLineParser.parse(this, args);

            if (cmdLine.hasOption("write-limit")) {
                softLineLimit = true;
                softLineLimitValue = ((Number)cmdLine.getParsedOptionValue("write-limit")).intValue();
            }

            if (cmdLine.hasOption("SPS")) {
                forcedSPS = true;
                SPS = ((Number)cmdLine.getParsedOptionValue("SPS")).floatValue();
            }

            if (cmdLine.hasOption("channel")) {
                channel = ((Number)cmdLine.getParsedOptionValue("channel")).intValue();
            }

            if (cmdLine.hasOption("data-output")) {
                outputDataFilePath = (String) cmdLine.getParsedOptionValue("data-output");
            }

            if (cmdLine.hasOption("input")) {
                inputFilePath = (String) cmdLine.getParsedOptionValue("input");
            }

            if (cmdLine.hasOption("inputWithSequenceNumber")) {
                inputWithSequenceNumber = true;
            }

            if (cmdLine.hasOption("outputWithTimeSinceEpoch")) {
                outputWithTimeSinceEpoch = true;
            }

            if (cmdLine.hasOption("isInputJson")) {
                isInputJson = true;
            }

            if (cmdLine.hasOption("debug")) {
                debug = true;
            }

            if (cmdLine.hasOption("only-check")) {
                onlyCheck = true;
            }

            if (cmdLine.hasOption("help")) {
                System.out.println("Help");
                HelpFormatter h = new HelpFormatter();
                h.printHelp("help", this);
                System.exit(-1);
            }

        } catch (ParseException e) {
            HelpFormatter h = new HelpFormatter();
            h.printHelp("help", this);
            System.exit(-1);
        }
    }

    @Override
    public String toString() {
        return String.format("Options:%n\thas line Limit:\t%s%n\tWrite line limit:\t%d%n\tdata-output:\t%s%n\tinput filepath:\t%s" + 
                "%n\tdebug:\t%s%n\toutputWithTimeSinceEpoch:\t%s%n\tinputWithSequenceNumber:\t%s%n\tIsInputWithJson: %s%n\tSPS: %f%n\tChannel: %d", 
                softLineLimit, softLineLimitValue, outputDataFilePath, inputFilePath, 
                debug, outputWithTimeSinceEpoch, inputWithSequenceNumber, isInputJson, SPS, channel);
    };
}
