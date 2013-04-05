package com.inescid.cnm;

import org.apache.commons.cli.CommandLine;
import org.apache.commons.cli.CommandLineParser;
import org.apache.commons.cli.OptionBuilder;
import org.apache.commons.cli.Options;
import org.apache.commons.cli.ParseException;
import org.apache.commons.cli.PosixParser;

public class ReaderCliOptions extends Options
{

    private static final long serialVersionUID = 1L;
    public String outputDataFilePath = "out.data";
    public Boolean softLineLimit = false;
    public int softLineLimitValue = -1;
    public final Options options = new Options();

    public ReaderCliOptions()
    {
        super();
    }

    void parse(String[] args)
    {
        CommandLineParser cmdLineParser = new PosixParser();

        options.addOption(OptionBuilder.withLongOpt("soft-line-limit")
                .withDescription("description")
                .withType(Number.class)
                .hasArg()
                .withArgName("l")
                .create());

        options.addOption(OptionBuilder.withLongOpt("data-output")
                .withDescription("description")
                .withType(String.class)
                .hasArg()
                .withArgName("o")
                .create());

        try {
            CommandLine cmdLine = cmdLineParser.parse(options, args);

            if (cmdLine.hasOption("soft-line-limit")) {
                softLineLimit = true;
                softLineLimitValue = ((Number)cmdLine.getParsedOptionValue("soft-line-limit")).intValue();
            }

            if (cmdLine.hasOption("data-output")) {
                outputDataFilePath = (String) cmdLine.getParsedOptionValue("data-output");
            }

        } catch (ParseException e) {
            e.printStackTrace();
        }
    }
}
