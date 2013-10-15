/**
 * Provides the class for communicating with RTS2 through JSON interface.
 *
 * If you want to use the package, you first need to create JSON object instance. Then you can use JSON object methods
 * to access RTS2 JSON values.
 *
 * <pre>
 * {@code import org.rts2.JSON;

class TestApp
{
	public static void main(String[] args) throws Exception
	{
		JSON json = new JSON("http://localhost:8889");
		System.out.println(json.getValue("centrald", "infotime"));
	}
}
 * }
 * </pre>
 *
 * Please visit <a href="http://rts2.org">rts2.org</a> for more information about the mighty RTS2 package.
 *
 * @author Petr Kubánek <petr@kubanek.net>
 *
 * @since 0.1
 */
package org.rts2;
