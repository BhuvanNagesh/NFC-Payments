<?php
header('Content-Type: application/json');
include "./db_conn.php";

// Check for DB connection errors
if ($conn->connect_error) {
    echo json_encode(array("status" => "error", "message" => "DB connection failed: " . $conn->connect_error));
    exit();
}

// 1. Get the specific transaction ID from the poller
if (!isset($_GET['tid'])) {
    echo json_encode(array("status" => "error", "message" => "No Transaction ID (tid) provided to poller."));
    exit();
}
// Sanitize the ID (it comes from time(), so it's an integer)
$transaction_id = (int)$_GET['tid'];

// 2. Prepare and execute a query for *that specific transaction*
$stmt = $conn->prepare("SELECT reads_card FROM payment WHERE transaction_id = ?");
$stmt->bind_param("i", $transaction_id); // "i" for integer

$response = array();

if ($stmt->execute()) {
    $result = $stmt->get_result();
    if ($result->num_rows == 0) {
        $response = array("status" => "error", "message" => "Transaction ID not found.");
    } else {
        $row = $result->fetch_assoc();
        
        // 3. Check the status of THAT row
        if ($row['reads_card'] == 1) {
            $response = array("status" => "success", "message" => "Payment successful");
        } else {
            $response = array("status" => "waiting", "message" => "Payment in progress...");
        }
    }
} else {
    $response = array("status" => "error", "message" => "Database query failed: " . $conn->error);
}

$stmt->close();
$conn->close();
echo json_encode($response);
?>