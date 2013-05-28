package org.rts2;

import junit.framework.Test;
import junit.framework.TestCase;
import junit.framework.TestSuite;

import org.rts2.JSON;

/**
 * Unit test for simple App.
 */
public class AppTest extends TestCase
{
	/**
	 * Create the test case
	 *
	 * @param testName name of the test case
	 */
	public AppTest(String testName)
	{
		super(testName);
	}

	/**
	 * @return the suite of tests being tested
	 */
	public static Test suite()
	{
		return new TestSuite( AppTest.class );
	}

	/**
	 * Rigourous Test :-)
	 */
	public void testApp() throws Exception
	{
		JSON json = new JSON("http://localhost:8889", "petr", "test");
		assertEquals("infotime value", "2012", json.getValue("C0", "infotime"));
	}
}
