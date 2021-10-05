<!DOCTYPE html PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN" "http://www.w3.org/TR/html4/loose.dtd">

<html>
<head>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8">
<title>MessageSight OAuth Login</title>
</head>

<body>
	<h3>Enter your username and password to login</h3>
	
	<form action="j_security_check" method="post">
		<table>
			<tr>
				<td>Username: </td>
				<td><input name="j_username" type="text" size="25">
				</td>
			</tr>
			<tr>
				<td>Password: </td>
				<td><input name="j_password" type="password" size="25">
				</td>
			</tr>
			<tr>
				<td colspan="2">
					<button type="submit" name="submitButton">Login</button>
					<button type="reset" name="resetButton">Reset</button>
				</td>
			</tr>
		</table>
	</form>
</body>
</html>