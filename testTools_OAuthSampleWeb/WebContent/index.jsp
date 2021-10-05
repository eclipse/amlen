<!DOCTYPE html PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN" "http://www.w3.org/TR/html4/loose.dtd">
<html>
<head>
<meta http-equiv="Content-Type" content="text/html; charset=UTF-8">
<title>MessageSight OAuth Application</title>
</head>
<body>
	<center>
		<h1>MessageSight OAuth Application</h1>
		<p>
			To generate an access token for MessageSight, please go to: <br />
			<a href="GetOAuthCode">Generate Access Token</a>
		</p>
		<p>
			Use the following to configure an OAuthProfile in MessageSight to use this application:<br/>
			<code>imaserver create OAuthProfile Name=OAuthProfile KeyFileName=<br/>
			ResourceURL=https://[WAS_SERVER_IP]:[PORT]/MessageSightOAuth/Home.jsp TokenNameProperty=access_token<br/>
			UserInfoURL=https://[WAS_SERVER_IP]:[PORT]/MessageSightOAuth/UserInfo.jsp UsernameProperty=username</code>
		</p>
		<p>
			Home.jsp and UserInfo.jsp are protected resources and require an access_token to view.
		</p>
	</center>
</body>
</html>