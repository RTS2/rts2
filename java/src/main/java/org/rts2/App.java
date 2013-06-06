package org.rts2;

import org.apache.commons.cli.BasicParser;
import org.apache.commons.cli.CommandLine;
import org.apache.commons.cli.CommandLineParser;
import org.apache.commons.cli.Options;

import org.rts2.JSON;

/**
 * Allows access to RTS2 through JSON. An application demonstrating org.rts2.JSON class capabilities.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
public class App 
{
    public static void main( String[] args ) throws Exception
    {
        Options options = new Options();

	options.addOption("G", true, "get value from a device");
	options.addOption("u", "url", true, "server URL (default to http://localhost:8889");

	CommandLineParser parser = new BasicParser();
	CommandLine cmd = parser.parse(options, args);

	JSON json = new JSON(cmd.getOptionValue("url","http://localhost:8889"), "petr", "test");

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
