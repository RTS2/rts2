package org.rts2;

import org.apache.commons.cli.BasicParser;
import org.apache.commons.cli.CommandLine;
import org.apache.commons.cli.CommandLineParser;
import org.apache.commons.cli.HelpFormatter;
import org.apache.commons.cli.Options;

import org.rts2.JSON;

/**
 * Allows access to RTS2 through JSON. An application demonstrating org.rts2.JSON class capabilities.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class App 
{
    public static void main( String[] args ) throws Exception
    {
        Options options = new Options();

	options.addOption("G", true, "get value from a device");
	options.addOption("u", "url", true, "server URL (default to http://localhost:8889");
	options.addOption("help", false, "print this help message");
	options.addOption("user", true, "JSON login");
	options.addOption("password", true, "JSON user password");

	CommandLineParser parser = new BasicParser();
	CommandLine cmd = parser.parse(options, args);

	if (cmd.hasOption ("help"))
	{
		HelpFormatter formatter = new HelpFormatter();
		formatter.printHelp("rts2-jjson", options);
		return;
	}

	JSON json;

	try
	{
		json = new JSON(cmd.getOptionValue("url","http://localhost:8889"), cmd.getOptionValue("user"), cmd.getOptionValue("password"));
	}
	catch (IllegalArgumentException arg)
	{
		json = new JSON(cmd.getOptionValue("url","http://localhost:8889"));
	}

	if (cmd.hasOption("G"))
	{
		// parse device,..
		String arg = cmd.getOptionValue("G");
		int dot = arg.indexOf('.');
		if (dot < 0)
		{
			System.err.println("You need to specify device and value, separated with '.' (a dot)");
			System.exit(1);
		}
		System.out.println(json.getValue(arg.substring(0,dot), arg.substring(dot+1)));
	}
    }
}
