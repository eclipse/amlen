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
	

	// Just hardcode some group info for users...
	if (username.equals("LTPAUser1")) {
		json.put("group", "LTPAGroup1");
	} else if (username.equals("LTPAUser2")) {
		json.put("group", "LTPAGroup2,LTPAGroup4");
	} else if (username.equals("LTPAUser3")) {
		json.put("group", "LTPAGroup3");
	} else if (username.equals("admin")) {
		json.put("group", "Comma,Group");
	} else if (username.equals("LTPAUser5")) {
		json.put("group", "Escaped\\,Comma");
	} else if (username.equals("LTPAUser6")) {
		json.put("group", "Unescaped,Comma");
	} else if (username.equals("LTPAUser7")) {
		json.put("group", "Double,,Comma");
	} else if (username.equals("LTPAUser8")) {
		json.put("group", "Escaped,Comma");
	} else if (username.equals("LTPAUser9")) {
		json.put("group", "LTPAGroup9");
	}
	
	pw.println(json.serialize());
	pw.close();
%>
</body>
</html>
