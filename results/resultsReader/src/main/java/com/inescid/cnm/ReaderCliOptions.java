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

    public String mseedPath = DEFAULT_INPUT_MSEED_PATH;
    public String outputDataFilePath = "out.data";
    public Boolean softLineLimit = false;
    public int softLineLimitValue = -1;

    public ReaderCliOptions()
    {
        super();

        OptionBuilder.withLongOpt("soft-line-limit");
        OptionBuilder.withDescription("Limits the number of mseed lines to process (only a soft limit, number of lines limit can be higher)");
        OptionBuilder.withType(Number.class);
        OptionBuilder.hasArg();
        OptionBuilder.withArgName("l");
        this.addOption(OptionBuilder.create());

        OptionBuilder.withLongOpt("mseed-path");
        OptionBuilder.withDescription("Input mseed filepath");
        OptionBuilder.withType(String.class);
        OptionBuilder.hasArg();
        OptionBuilder.withArgName("i");
        this.addOption(OptionBuilder.create());

        OptionBuilder.withLongOpt("data-output");
        OptionBuilder.withDescription("Filepath to write the data output");
        OptionBuilder.withType(String.class);
        OptionBuilder.hasArg();
        OptionBuilder.withArgName("o");
        this.addOption(OptionBuilder.create());

        OptionBuilder.withLongOpt("help");
        OptionBuilder.withDescription("print this message and exit");
        OptionBuilder.withArgName("h");
        this.addOption(OptionBuilder.create());

    }

    void parse(String[] args)
    {
        CommandLineParser cmdLineParser = new PosixParser();

        try {
            CommandLine cmdLine = cmdLineParser.parse(this, args);

            if (cmdLine.hasOption("soft-line-limit")) {
                System.out.println("Has option soft");
                softLineLimit = true;
                softLineLimitValue = ((Number)cmdLine.getParsedOptionValue("soft-line-limit")).intValue();
            }

            if (cmdLine.hasOption("data-output")) {
                System.out.println("Data output");
                softLineLimit = true;
                outputDataFilePath = (String) cmdLine.getParsedOptionValue("data-output");
            }

            if (cmdLine.hasOption("mseed-path")) {
                System.out.println("Mseed path");
                mseedPath = (String) cmdLine.getParsedOptionValue("mseed-path");
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
}
