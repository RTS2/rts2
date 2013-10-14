package org.rts2;

import java.lang.String;
import java.lang.StringBuilder;

import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.io.IOException;

import java.net.URL;
import java.util.Date;

import org.apache.http.HttpResponse;
import org.apache.http.impl.client.DefaultHttpClient;
import org.apache.http.auth.AuthScope;
import org.apache.http.auth.UsernamePasswordCredentials;
import org.apache.http.client.methods.HttpGet;
import org.apache.http.client.utils.URIBuilder;
import org.apache.http.params.BasicHttpParams;
import org.apache.http.params.CoreProtocolPNames;

import org.json.JSONObject;

/**
 * Class to access RTS2 via JAVA objects. Provides method to extract data from
 * a single RTS2 server.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class JSON
{
	/**
         * Construct connection to given URL.
         *
         * @param url Server URL
	 */
	public JSON(String url) throws Exception
	{
		this(url,null,null);
	}

	/**
         * Construct connection to given URL, using given username and password.
         *
         * @param url Server URL
         * @param login Server login
	 * @param password User password 
         */
        public JSON(String url, String login, String password) throws Exception
	{
		client = new DefaultHttpClient();
		client.getParams().setParameter(CoreProtocolPNames.USER_AGENT, "java-rts2");

		URL parsedUrl = new URL(url);

		int port = parsedUrl.getPort();
		if (port == -1) {
			port = parsedUrl.getDefaultPort();
		}

		if (login != null && password != null)
		{
			client.getCredentialsProvider().setCredentials(new AuthScope(parsedUrl.getHost(), port), new UsernamePasswordCredentials(login, password));
		}

		connect (url);
	}

	/**
	 * Initiate connection to a server.
	 */
	public void connect(String url)
	{
		baseUrl = url;
	}

	/**
	 * Return value from the given device.
	 */
	public String getValue(String device, String value) throws Exception
	{
		HttpGet request = new HttpGet((new URIBuilder(baseUrl + "/api/get")).addParameter("d",device).build());
	
		BasicHttpParams httpParams = new BasicHttpParams();
		httpParams.setParameter("device", device);

		request.setParams(httpParams);

		HttpResponse response = client.execute(request);
		BufferedReader reader = new BufferedReader(new InputStreamReader(response.getEntity().getContent()), 8);

		StringBuilder sb = new StringBuilder();

		String line;
		while ((line = reader.readLine()) != null) {
			sb.append(line + "\n");
		}

		JSONObject json = new JSONObject(sb.toString());
		return json.getJSONObject("d").get(value).toString();
	}

	/**
	 * Returns double value from JSON.
	 */
	public double getValueDouble(String device, String value) throws Exception
	{
		return Double.parseDouble(getValue(device, value));
	}

	public Date getValueDate(String device, String value) throws Exception
	{
		return new Date((long)getValueDouble(device, value));
	}

	private DefaultHttpClient client;
	private String baseUrl;
}
