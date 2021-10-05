<%@ page language="java" contentType="text/html; charset=ISO-8859-1"
    pageEncoding="ISO-8859-1"%>
<!DOCTYPE html PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN" "http://www.w3.org/TR/html4/loose.dtd">
<html>
<head>
<meta http-equiv="Content-Type" content="text/html; charset=ISO-8859-1">
<title>MessageSight OAuth 2.0</title>
</head>
<body>
You are authorized to connect to MessageSight
<p>
Access Token:
<%= request.getSession().getAttribute("ACCESS_TOKEN_JSON") %> <p>
Arbitrary Header:
The user agent is <%= request.getHeader("user-agent") %>
<p>
Implicit Headers: <br>
Request method:
<%= request.getMethod() %> <p>
Request URI:
<%= request.getRequestURI() %> <p>
Request protocol:
<%= request.getProtocol() %> <p>
Remote Host:
<%= request.getRemoteHost() %> <p>
Remote Address:
<%= request.getRemoteAddr() %> <p>


</body>
</html>