package org.rts2;

import java.lang.String;
import java.lang.StringBuilder;

import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.io.IOException;

import java.net.URL;
import java.net.MalformedURLException;

import org.apache.http.HttpResponse;
import org.apache.http.impl.client.DefaultHttpClient;
import org.apache.http.auth.AuthScope;
import org.apache.http.auth.UsernamePasswordCredentials;
import org.apache.http.client.methods.HttpGet;
import org.apache.http.params.BasicHttpParams;
import org.apache.http.params.CoreProtocolPNames;

import org.json.JSONObject;
import org.json.JSONException;

/**
 * Class to access RTS2 via JAVA objects. Provides method to extract data from
 * a single RTS2 server.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class JSON
{
	public JSON(String url, String login, String password) throws MalformedURLException
	{
		client = new DefaultHttpClient();
		client.getParams().setParameter(CoreProtocolPNames.USER_AGENT, "java-rts2");

		URL parsedUrl = new URL(url);

		int port = parsedUrl.getPort();
		if (port == -1) {
			port = parsedUrl.getDefaultPort();
		}

		client.getCredentialsProvider().setCredentials(new AuthScope(parsedUrl.getHost(), port), new UsernamePasswordCredentials(login, password));

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
	public String getValue(String device, String value) throws JSONException,IOException
	{
		HttpGet request = new HttpGet(baseUrl + "/api/get");
	
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
		return json.get(value).toString();
	}

	private DefaultHttpClient client;
	private String baseUrl;
}
