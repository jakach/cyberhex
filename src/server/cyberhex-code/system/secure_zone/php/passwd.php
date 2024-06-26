<?php
session_start();

// Check if the user is logged in
if (!isset($_SESSION['username']) or !isset($_SESSION["login"])) {
    // Redirect to the login page or handle unauthorized access
    header("Location: /login.php");
    exit();
}

$username = $_SESSION['username'];
$perms = $_SESSION["perms"];
$email = $_SESSION["email"];
include "../../../api/php/log/add_server_entry.php"; //to log things
?>
<!DOCTYPE html>
<html lang="en">
<head>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <link href="https://cdn.jsdelivr.net/npm/bootstrap@5.0.2/dist/css/bootstrap.min.css" rel="stylesheet" integrity="sha384-EVSTQN3/azprG1Anm3QDgpJLIm9Nao0Yz1ztcQTwFspd3yD65VohhpuuCOmLASjC" crossorigin="anonymous">
    <script src="https://cdn.jsdelivr.net/npm/bootstrap@5.0.2/dist/js/bootstrap.bundle.min.js" integrity="sha384-MrcW6ZMFYlzcLA8Nl+NtUVF0sA7MsXsP1UyJoMp4YLEuNSfAP+JcXn/tWtIaxVXM" crossorigin="anonymous"></script>
	 <title>Change Password</title>
</head>
<body>
<script>
//js to handle passkey
	async function createRegistration() {
            try {

                // check browser support
                if (!window.fetch || !navigator.credentials || !navigator.credentials.create) {
                    throw new Error('Browser not supported.');
                }

                // get create args
                let rep = await window.fetch('/system/insecure_zone/php/add_user_passkey.php?fn=getCreateArgs' + getGetParams(), {method:'GET', cache:'no-cache'});
                const createArgs = await rep.json();

                // error handling
                if (createArgs.success === false) {
                    throw new Error(createArgs.msg || 'unknown error occured');
                }

                // replace binary base64 data with ArrayBuffer. a other way to do this
                // is the reviver function of JSON.parse()
                recursiveBase64StrToArrayBuffer(createArgs);

                // create credentials
                const cred = await navigator.credentials.create(createArgs);

                // create object
                const authenticatorAttestationResponse = {
                    transports: cred.response.getTransports  ? cred.response.getTransports() : null,
                    clientDataJSON: cred.response.clientDataJSON  ? arrayBufferToBase64(cred.response.clientDataJSON) : null,
                    attestationObject: cred.response.attestationObject ? arrayBufferToBase64(cred.response.attestationObject) : null
                };

                // check auth on server side
                rep = await window.fetch('/system/insecure_zone/php/add_user_passkey.php?fn=processCreate' + getGetParams(), {
                    method  : 'POST',
                    body    : JSON.stringify(authenticatorAttestationResponse),
                    cache   : 'no-cache'
                });
                const authenticatorAttestationServerResponse = await rep.json();

                // prompt server response
                if (authenticatorAttestationServerResponse.success) {
                    reloadServerPreview();
                    window.alert(authenticatorAttestationServerResponse.msg || 'registration success');
                } else {
                    throw new Error(authenticatorAttestationServerResponse.msg);
                }

            } catch (err) {
                reloadServerPreview();
                window.alert(err.message || 'unknown error occured');
            }
        }



        function queryFidoMetaDataService() {
            window.fetch('/system/insecure_zone/php/add_user_passkey.php?fn=queryFidoMetaDataService' + getGetParams(), {method:'GET',cache:'no-cache'}).then(function(response) {
                return response.json();

            }).then(function(json) {
               if (json.success) {
                   window.alert(json.msg);
               } else {
                   throw new Error(json.msg);
               }
            }).catch(function(err) {
                window.alert(err.message || 'unknown error occured');
            });
        }

        /**
         * convert RFC 1342-like base64 strings to array buffer
         * @param {mixed} obj
         * @returns {undefined}
         */
        function recursiveBase64StrToArrayBuffer(obj) {
            let prefix = '=?BINARY?B?';
            let suffix = '?=';
            if (typeof obj === 'object') {
                for (let key in obj) {
                    if (typeof obj[key] === 'string') {
                        let str = obj[key];
                        if (str.substring(0, prefix.length) === prefix && str.substring(str.length - suffix.length) === suffix) {
                            str = str.substring(prefix.length, str.length - suffix.length);

                            let binary_string = window.atob(str);
                            let len = binary_string.length;
                            let bytes = new Uint8Array(len);
                            for (let i = 0; i < len; i++)        {
                                bytes[i] = binary_string.charCodeAt(i);
                            }
                            obj[key] = bytes.buffer;
                        }
                    } else {
                        recursiveBase64StrToArrayBuffer(obj[key]);
                    }
                }
            }
        }

        /**
         * Convert a ArrayBuffer to Base64
         * @param {ArrayBuffer} buffer
         * @returns {String}
         */
        function arrayBufferToBase64(buffer) {
            let binary = '';
            let bytes = new Uint8Array(buffer);
            let len = bytes.byteLength;
            for (let i = 0; i < len; i++) {
                binary += String.fromCharCode( bytes[ i ] );
            }
            return window.btoa(binary);
        }
		
		function ascii_to_hex(str) {
			let hex = '';
			for (let i = 0; i < str.length; i++) {
				let ascii = str.charCodeAt(i).toString(16);
				hex += ('00' + ascii).slice(-2); // Ensure each hex value is 2 characters long
			}
			return hex;
		}
        /**
         * Get URL parameter
         * @returns {String}
         */
        function getGetParams() {
            let url = '';

            url += '&apple=1';
            url += '&yubico=1';
            url += '&solo=1'
            url += '&hypersecu=1';
            url += '&google=1';
            url += '&microsoft=1';
            url += '&mds=1';

            url += '&requireResidentKey=0';

            url += '&type_usb=1';
            url += '&type_nfc=1';
            url += '&type_ble=1';
            url += '&type_int=1';
            url += '&type_hybrid=1';

            url += '&fmt_android-key=1';
            url += '&fmt_android-safetynet=1';
            url += '&fmt_apple=1';
            url += '&fmt_fido-u2f=1';
            url += '&fmt_none=1';
            url += '&fmt_packed=1';
            url += '&fmt_tpm=1';

            url += '&rpId=auth.jakach.com';
			
            url += '&userId=' + encodeURIComponent(ascii_to_hex('<?php echo($username);?>'));
            url += '&userName=' + encodeURIComponent('<?php echo($username);?>');
            url += '&userDisplayName=' + encodeURIComponent('<?php echo($username);?>');

            url += '&userVerification=discouraged';

            return url;
        }

        function reloadServerPreview() {
            //let iframe = document.getElementById('serverPreview');
            //iframe.src = iframe.src;
        }

        function setAttestation(attestation) {
            let inputEls = document.getElementsByTagName('input');
            for (const inputEl of inputEls) {
                if (inputEl.id && inputEl.id.match(/^(fmt|cert)\_/)) {
                    inputEl.disabled = !attestation;
                }
                if (inputEl.id && inputEl.id.match(/^fmt\_/)) {
                    inputEl.checked = attestation ? inputEl.id !== 'fmt_none' : inputEl.id === 'fmt_none';
                }
                if (inputEl.id && inputEl.id.match(/^cert\_/)) {
                    inputEl.checked = attestation ? inputEl.id === 'cert_mds' : false;
                }
            }
        }

        /**
         * force https on load
         * @returns {undefined}
         */
        window.onload = function() {
            if (location.protocol !== 'https:' && location.host !== 'localhost') {
                location.href = location.href.replace('http', 'https');
            }
        }
</script>
<div class="container mt-5">
    <div class="row justify-content-center">
        <div class="col-md-6">
            <div class="card">
                <div class="card-header">
                    <h4>Change Password (<?php echo($username); ?>)</h4>
                </div>
                <div class="card-body">
					<form action="passwd.php?update=true" method="post">
                        <div class="form-group">
                            <label for="password">Password:</label>
                            <input type="password" class="form-control" id="password" name="password" required>
                        </div>
						<div class="form-group">
                            <label for="password">New Password:</label>
                            <input type="password" class="form-control" id="new_password1" name="new_password1" required>
                        </div>
						<div class="form-group">
                            <label for="password">Repeat New Password:</label>
                            <input type="password" class="form-control" id="new_password2" name="new_password2" required>
                        </div>
						<br>
                        <button type="submit" class="btn btn-primary btn-block">Update</button>
						<br>
						Or
						<br>
						<button type="button" class="btn btn-primary btn-block" onclick="createRegistration()">Add a passkey</button>
                    </form>
					<br>
					<!-- php code to verify password-->
					<?php
						// Check if the form is submitted
						if ($_SERVER["REQUEST_METHOD"] == "POST") {
							//include db pw
							include "../../../config.php";
							
							// Retrieve user input
							$password = $_POST["password"];
							$new_password1=$_POST["new_password1"];
							$new_password2=$_POST["new_password2"];
							$hash=password_hash($new_password1, PASSWORD_BCRYPT);
							// Create a connection
							$conn = new mysqli($DB_SERVERNAME, $DB_USERNAME, $DB_PASSWORD, $DB_DATABASE);

							// Check the connection
							if ($conn->connect_error) {
								die("Connection failed: " . $conn->connect_error);
							}
							$sql = "SELECT * FROM users WHERE username = ?";
							$stmt = $conn->prepare($sql);
							$stmt->bind_param("s", $username);
							
							// Execute the statement
							$stmt->execute();

							// Get the result
							$result = $stmt->get_result();
							$stmt->close();
							$conn->close();
							
							
							// Check if the user exists and verify the password
							if($new_password1===$new_password2){
								if ($result->num_rows > 0) {
									$row = $result->fetch_assoc();
									if (password_verify($password, $row['password'])) {
										//password correct update
										// Create connection
										$conn = new mysqli($DB_SERVERNAME, $DB_USERNAME, $DB_PASSWORD,$DB_DATABASE);

										// Check connection
										if ($conn->connect_error) {
											$success=0;
											die("Connection failed: " . $conn->connect_error);
										}
										$stmt = $conn->prepare("UPDATE users set password = ? where username = ?");
										$stmt->bind_param("ss", $hash, $username);
										$stmt->execute();
										$stmt->close();
										$conn->close();
										log_action("PASSWD::CHANGE::SUCCESS","User ".$_SESSION["username"]." changed his password.",$_SESSION["id"]);
										echo '<br><div class="alert alert-success" role="alert">
											Information updated successfully!
										  </div>';
										
									} else {
										log_action("PASSWD::CHANGE::FAILURE","User ".$_SESSION["username"]." tried to change his password but failed due to wrong password.",$_SESSION["id"]);
										echo '<div class="alert alert-danger" role="alert">
												Incorrect password.
											  </div>';
									}
								} else {
									log_action("PASSWD::CHANGE::FAILURE","User ".$_SESSION["username"]." tried to change his password but failed due to wrong password.",$_SESSION["id"]);
									echo '<div class="alert alert-danger" role="alert">
											Incorrect password.
										  </div>';
								}
							}else{
								echo '<div class="alert alert-danger" role="alert">
											New password does not match.
										  </div>';
							}

						}
					?>
					
                </div>
            </div>
        </div>
    </div>
</div>
</body>
</html>
