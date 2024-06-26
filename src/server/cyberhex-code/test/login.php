<?php
/*
require_once 'WebAuthn.php';
try {
    session_start();

    // read get argument and post body
    $fn = filter_input(INPUT_GET, 'fn');
    $requireResidentKey = !!filter_input(INPUT_GET, 'requireResidentKey');
    $userVerification = filter_input(INPUT_GET, 'userVerification', FILTER_SANITIZE_SPECIAL_CHARS);

    $userId = filter_input(INPUT_GET, 'userId', FILTER_SANITIZE_SPECIAL_CHARS);
    $userName = filter_input(INPUT_GET, 'userName', FILTER_SANITIZE_SPECIAL_CHARS);
    $userDisplayName = filter_input(INPUT_GET, 'userDisplayName', FILTER_SANITIZE_SPECIAL_CHARS);

    $userId = preg_replace('/[^0-9a-f]/i', '', $userId);
    $userName = preg_replace('/[^0-9a-z]/i', '', $userName);
    $userDisplayName = preg_replace('/[^0-9a-z öüäéèàÖÜÄÉÈÀÂÊÎÔÛâêîôû]/i', '', $userDisplayName);

    $post = trim(file_get_contents('php://input'));
    if ($post) {
        $post = json_decode($post, null, 512, JSON_THROW_ON_ERROR);
    }

    if ($fn !== 'getStoredDataHtml') {

        // Formats
        $formats = [];
        //if (filter_input(INPUT_GET, 'fmt_android-key')) {
            $formats[] = 'android-key';
        //}
        ///if (filter_input(INPUT_GET, 'fmt_android-safetynet')) {
            $formats[] = 'android-safetynet';
        //}
        //if (filter_input(INPUT_GET, 'fmt_apple')) {
            $formats[] = 'apple';
        //}
        //if (filter_input(INPUT_GET, 'fmt_fido-u2f')) {
            $formats[] = 'fido-u2f';
        //}
        //if (filter_input(INPUT_GET, 'fmt_none')) {
            $formats[] = 'none';
        //}
        //if (filter_input(INPUT_GET, 'fmt_packed')) {
            $formats[] = 'packed';
        //}
        //if (filter_input(INPUT_GET, 'fmt_tpm')) {
            $formats[] = 'tpm';
        //}

		$rpId=$_SERVER['SERVER_NAME'];
		
		$typeUsb = true;
		$typeNfc = true;
		$typeBle = true;
		$typeInt = true;
		$typeHyb = true;

        // cross-platform: true, if type internal is not allowed
        //                 false, if only internal is allowed
        //                 null, if internal and cross-platform is allowed
        $crossPlatformAttachment = null;
        if (($typeUsb || $typeNfc || $typeBle || $typeHyb) && !$typeInt) {
            $crossPlatformAttachment = true;

        } else if (!$typeUsb && !$typeNfc && !$typeBle && !$typeHyb && $typeInt) {
            $crossPlatformAttachment = false;
        }


        // new Instance of the server library.
        // make sure that $rpId is the domain name.
        $WebAuthn = new lbuchs\WebAuthn\WebAuthn('WebAuthn Library', $rpId, $formats);

        // add root certificates to validate new registrations
        //if (filter_input(INPUT_GET, 'solo')) {
            $WebAuthn->addRootCertificates('rootCertificates/solo.pem');
        //}
        //if (filter_input(INPUT_GET, 'apple')) {
            $WebAuthn->addRootCertificates('rootCertificates/apple.pem');
        //}
        //if (filter_input(INPUT_GET, 'yubico')) {
            $WebAuthn->addRootCertificates('rootCertificates/yubico.pem');
        //}
        //if (filter_input(INPUT_GET, 'hypersecu')) {
            $WebAuthn->addRootCertificates('rootCertificates/hypersecu.pem');
        //}
        //if (filter_input(INPUT_GET, 'google')) {
            $WebAuthn->addRootCertificates('rootCertificates/globalSign.pem');
            $WebAuthn->addRootCertificates('rootCertificates/googleHardware.pem');
        //}
        //if (filter_input(INPUT_GET, 'microsoft')) {
            $WebAuthn->addRootCertificates('rootCertificates/microsoftTpmCollection.pem');
        //}
        //if (filter_input(INPUT_GET, 'mds')) {
            $WebAuthn->addRootCertificates('rootCertificates/mds');
        //}

    }

    // ------------------------------------
    // request for create arguments
    // ------------------------------------

    if ($fn === 'getCreateArgs') {
        $createArgs = $WebAuthn->getCreateArgs(\hex2bin($userId), $userName, $userDisplayName, 60*4, $requireResidentKey, $userVerification, $crossPlatformAttachment);

        header('Content-Type: application/json');
        print(json_encode($createArgs));

        // save challange to session. you have to deliver it to processGet later.
        $_SESSION['challenge'] = $WebAuthn->getChallenge();



    // ------------------------------------
    // request for get arguments
    // ------------------------------------

    } else if ($fn === 'getGetArgs') {
        $ids = [];

        if ($requireResidentKey) {
            if (!isset($_SESSION['registrations']) || !is_array($_SESSION['registrations']) || count($_SESSION['registrations']) === 0) {
                throw new Exception('we do not have any registrations in session to check the registration');
            }

        } else {
            // load registrations from session stored there by processCreate.
            // normaly you have to load the credential Id's for a username
            // from the database.
            if (isset($_SESSION['registrations']) && is_array($_SESSION['registrations'])) {
                foreach ($_SESSION['registrations'] as $reg) {
                    if ($reg->userId === $userId) {
                        $ids[] = $reg->credentialId;
                    }
                }
            }

            if (count($ids) === 0) {
                throw new Exception('no registrations in session for userId ' . $userId);
            }
        }

        $getArgs = $WebAuthn->getGetArgs($ids, 60*4, $typeUsb, $typeNfc, $typeBle, $typeHyb, $typeInt, $userVerification);

        header('Content-Type: application/json');
        print(json_encode($getArgs));

        // save challange to session. you have to deliver it to processGet later.
        $_SESSION['challenge'] = $WebAuthn->getChallenge();

    }else if ($fn === 'processGet') {
        $clientDataJSON = base64_decode($post->clientDataJSON);
        $authenticatorData = base64_decode($post->authenticatorData);
        $signature = base64_decode($post->signature);
        $userHandle = base64_decode($post->userHandle);
        $id = base64_decode($post->id);
        $challenge = $_SESSION['challenge'] ?? '';
        $credentialPublicKey = null;

        // looking up correspondending public key of the credential id
        // you should also validate that only ids of the given user name
        // are taken for the login.
        if (isset($_SESSION['registrations']) && is_array($_SESSION['registrations'])) {
            foreach ($_SESSION['registrations'] as $reg) {
                if ($reg->credentialId === $id) {
                    $credentialPublicKey = $reg->credentialPublicKey;
                    break;
                }
            }
        }

        if ($credentialPublicKey === null) {
            throw new Exception('Public Key for credential ID not found!');
        }

        // if we have resident key, we have to verify that the userHandle is the provided userId at registration
        if ($requireResidentKey && $userHandle !== hex2bin($reg->userId)) {
            throw new \Exception('userId doesnt match (is ' . bin2hex($userHandle) . ' but expect ' . $reg->userId . ')');
        }

        // process the get request. throws WebAuthnException if it fails
        $WebAuthn->processGet($clientDataJSON, $authenticatorData, $signature, $credentialPublicKey, $challenge, null, $userVerification === 'required');

		//we have authenticated the user!
		

        $return = new stdClass();
        $return->success = true;

        header('Content-Type: application/json');
        print(json_encode($return));

    }
} catch (Throwable $ex) {
    $return = new stdClass();
    $return->success = false;
    $return->msg = $ex->getMessage();

    header('Content-Type: application/json');
    print(json_encode($return));
}
*/
?>
<?php
//with db:

require_once 'WebAuthn.php';
// Assuming you've already established a database connection here
include "../config.php";
$conn = new mysqli($DB_SERVERNAME, $DB_USERNAME, $DB_PASSWORD,$DB_DATABASE);
if ($conn->connect_error) {
	$success=0;
	die("Connection failed: " . $conn->connect_error);
}
try {
    session_start();

    // read get argument and post body
    $fn = filter_input(INPUT_GET, 'fn');
    $requireResidentKey = !!filter_input(INPUT_GET, 'requireResidentKey');
    $userVerification = filter_input(INPUT_GET, 'userVerification', FILTER_SANITIZE_SPECIAL_CHARS);

    $userId = filter_input(INPUT_GET, 'userId', FILTER_SANITIZE_SPECIAL_CHARS);
    $userName = filter_input(INPUT_GET, 'userName', FILTER_SANITIZE_SPECIAL_CHARS);
    $userDisplayName = filter_input(INPUT_GET, 'userDisplayName', FILTER_SANITIZE_SPECIAL_CHARS);

    $userId = preg_replace('/[^0-9a-f]/i', '', $userId);
    $userName = preg_replace('/[^0-9a-z]/i', '', $userName);
    $userDisplayName = preg_replace('/[^0-9a-z öüäéèàÖÜÄÉÈÀÂÊÎÔÛâêîôû]/i', '', $userDisplayName);

    $post = trim(file_get_contents('php://input'));
    if ($post) {
        $post = json_decode($post, null, 512, JSON_THROW_ON_ERROR);
    }

    if ($fn !== 'getStoredDataHtml') {

        // Formats
        $formats = [];
        //if (filter_input(INPUT_GET, 'fmt_android-key')) {
            $formats[] = 'android-key';
        //}
        ///if (filter_input(INPUT_GET, 'fmt_android-safetynet')) {
            $formats[] = 'android-safetynet';
        //}
        //if (filter_input(INPUT_GET, 'fmt_apple')) {
            $formats[] = 'apple';
        //}
        //if (filter_input(INPUT_GET, 'fmt_fido-u2f')) {
            $formats[] = 'fido-u2f';
        //}
        //if (filter_input(INPUT_GET, 'fmt_none')) {
            $formats[] = 'none';
        //}
        //if (filter_input(INPUT_GET, 'fmt_packed')) {
            $formats[] = 'packed';
        //}
        //if (filter_input(INPUT_GET, 'fmt_tpm')) {
            $formats[] = 'tpm';
        //}

		$rpId=$_SERVER['SERVER_NAME'];
		
		$typeUsb = true;
		$typeNfc = true;
		$typeBle = true;
		$typeInt = true;
		$typeHyb = true;

        // cross-platform: true, if type internal is not allowed
        //                 false, if only internal is allowed
        //                 null, if internal and cross-platform is allowed
        $crossPlatformAttachment = null;
        if (($typeUsb || $typeNfc || $typeBle || $typeHyb) && !$typeInt) {
            $crossPlatformAttachment = true;

        } else if (!$typeUsb && !$typeNfc && !$typeBle && !$typeHyb && $typeInt) {
            $crossPlatformAttachment = false;
        }


        // new Instance of the server library.
        // make sure that $rpId is the domain name.
        $WebAuthn = new lbuchs\WebAuthn\WebAuthn('WebAuthn Library', $rpId, $formats);

        // add root certificates to validate new registrations
        //if (filter_input(INPUT_GET, 'solo')) {
            $WebAuthn->addRootCertificates('rootCertificates/solo.pem');
        //}
        //if (filter_input(INPUT_GET, 'apple')) {
            $WebAuthn->addRootCertificates('rootCertificates/apple.pem');
        //}
        //if (filter_input(INPUT_GET, 'yubico')) {
            $WebAuthn->addRootCertificates('rootCertificates/yubico.pem');
        //}
        //if (filter_input(INPUT_GET, 'hypersecu')) {
            $WebAuthn->addRootCertificates('rootCertificates/hypersecu.pem');
        //}
        //if (filter_input(INPUT_GET, 'google')) {
            $WebAuthn->addRootCertificates('rootCertificates/globalSign.pem');
            $WebAuthn->addRootCertificates('rootCertificates/googleHardware.pem');
        //}
        //if (filter_input(INPUT_GET, 'microsoft')) {
            $WebAuthn->addRootCertificates('rootCertificates/microsoftTpmCollection.pem');
        //}
        //if (filter_input(INPUT_GET, 'mds')) {
            $WebAuthn->addRootCertificates('rootCertificates/mds');
        //}

    }

    // Handle different functions
    if ($fn === 'getCreateArgs') {
        // Get create arguments
        $createArgs = $WebAuthn->getCreateArgs(\hex2bin($userId), $userName, $userDisplayName, 60*4, $requireResidentKey, $userVerification);
        header('Content-Type: application/json');
        print(json_encode($createArgs));

        // Save challenge to session or somewhere else if needed
    } else if ($fn === 'getGetArgs') {
        $ids = [];

		//get registrations form user table
		//put credential id into session where userid = $userId
		
		$stmt = $conn->prepare("SELECT credential_id FROM users WHERE user_hex_id = ?");
		$stmt->bind_param("s", $userId);
        $stmt->execute();
		$registration = $stmt->get_result();
		$row = $registration->fetch_assoc();
		
        
        if ($registration->num_rows <= 0) {
            throw new Exception('User does not exist');
        }
		
		$_SESSION["registrations"]["credentialId"]=$row["credential_id"];
		
		$ids[]=$row["credential_id"];
        $_SESSION["registrations"]["userId"]=$userId;

        $getArgs = $WebAuthn->getGetArgs($ids, 60*4, $typeUsb, $typeNfc, $typeBle, $typeHyb, $typeInt, $userVerification);

        header('Content-Type: application/json');
        print(json_encode($getArgs));

        // save challange to session. you have to deliver it to processGet later.
        $_SESSION['challenge'] = $WebAuthn->getChallenge();

    }else if ($fn === 'processGet') {
        // Process get
        // Retrieve registration data from the database based on credential ID
        $id = base64_decode($post->id);
        $stmt = $conn->prepare("SELECT * FROM users WHERE credential_id = ?");
        $stmt->bind_param("s", $_SESSION["registrations"]["credentialId"]);
        $stmt->execute();
		$registration = $stmt->get_result();
		$row = $registration->fetch_assoc();
        
        if (!$registration) {
            throw new Exception('Public Key for credential ID not found!');
        }

        $clientDataJSON = base64_decode($post->clientDataJSON);
        $authenticatorData = base64_decode($post->authenticatorData);
        $signature = base64_decode($post->signature);
        $userHandle = base64_decode($post->userHandle);
        $challenge = $_SESSION['challenge'] ?? '';
        $credentialPublicKey = $row['public_key'];

        // Process the get request
        $WebAuthn->processGet($clientDataJSON, $authenticatorData, $signature, $credentialPublicKey, $challenge, null, $userVerification === 'required');

        // Authentication success
		//set sessionso user is authenticated
		$_SESSION["username"]=$row["username"];
		$_SESSION["login"]=true;
		$_SESSION["perms"]=$row["perms"];
		$_SESSION["email"]=$row["email"];
		$_SESSION["telegram_id"]=$row["telegram_id"];
		
        $return = new stdClass();
        $return->success = true;
        header('Content-Type: application/json');
        print(json_encode($return));
    }

} catch (Throwable $ex) {
    $return = new stdClass();
    $return->success = false;
    $return->msg = $ex->getMessage();

    header('Content-Type: application/json');
    print(json_encode($return));
}

?>
