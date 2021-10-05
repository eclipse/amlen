<%@page contentType="application/json; charset=UTF-8"%>
<%@page import="com.ibm.json.java.JSONObject"%>
<%@page import="java.io.PrintWriter"%>
<title>MessageSight OAuth 2.0 UserInfo</title>
<body>
<%
	PrintWriter pw = response.getWriter();
	JSONObject json = new JSONObject();
	
	String username = request.getRemoteUser();

	json.put("username",username);
	
	pw.println(json.serialize());
	pw.close();
%>
</body>
</html>