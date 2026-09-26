"""
AI-Powered Drowsiness & Yawn Detection Companion
------------------------------------------------
Uses Computer Vision and Deep Learning (MediaPipe Face Mesh + OpenCV)
to detect driver drowsiness in real-time using:
 1. Eye Aspect Ratio (EAR) - Detects prolonged eye closure / microsleeps.
 2. Mouth Aspect Ratio (MAR) - Detects frequent yawning (early fatigue sign).
 3. Telegram Bot Integration - Sends instant alerts with webcam photo snapshots.

Prerequisites:
  pip install opencv-python mediapipe requests numpy
"""

import time
import math
import cv2
import numpy as np
import requests

# ======================== CONFIGURATION ========================
TELEGRAM_BOT_TOKEN = "YOUR_TELEGRAM_BOT_TOKEN"
TELEGRAM_CHAT_ID   = "YOUR_TELEGRAM_CHAT_ID"

# Safety Thresholds
EAR_THRESHOLD         = 0.21    # Below this value: eye is closed
EAR_CONSEC_FRAMES     = 30      # ~1.0-1.5 seconds at 30 FPS
MAR_THRESHOLD         = 0.65    # Above this value: mouth is wide open (yawn)
MAR_CONSEC_FRAMES     = 35      # Sustained yawn duration
ALERT_COOLDOWN_SEC    = 20      # Cooldown between Telegram alert messages

# MediaPipe Landmark Indices for Eye and Lip contours
LEFT_EYE_LANDMARKS  = [362, 385, 387, 263, 373, 380]
RIGHT_EYE_LANDMARKS = [33,  160, 158, 133, 153, 144]
MOUTH_LANDMARKS     = [61, 291, 13, 14, 78, 308]
# ===============================================================

def send_telegram_alert(message, image_path=None):
    """Sends text notification and optional captured photo to Telegram."""
    try:
        if image_path:
            url = f"https://api.telegram.org/bot{TELEGRAM_BOT_TOKEN}/sendPhoto"
            with open(image_path, "rb") as photo:
                data = {"chat_id": TELEGRAM_CHAT_ID, "caption": message, "parse_mode": "Markdown"}
                response = requests.post(url, data=data, files={"photo": photo}, timeout=5)
        else:
            url = f"https://api.telegram.org/bot{TELEGRAM_BOT_TOKEN}/sendMessage"
            payload = {"chat_id": TELEGRAM_CHAT_ID, "text": message, "parse_mode": "Markdown"}
            response = requests.post(url, json=payload, timeout=5)
        print("Telegram alert sent! Status code:", response.status_code)
    except Exception as e:
        print(f"Failed to send Telegram alert: {e}")

def euclidean_dist(pt1, pt2):
    return math.hypot(pt1[0] - pt2[0], pt1[1] - pt2[1])

def calculate_ear(landmarks, eye_indices, width, height):
    """Calculates Eye Aspect Ratio (EAR) from 6 landmark points."""
    pts = [(int(landmarks[idx].x * width), int(landmarks[idx].y * height)) for idx in eye_indices]
    # Vertical distances
    v1 = euclidean_dist(pts[1], pts[5])
    v2 = euclidean_dist(pts[2], pts[4])
    # Horizontal distance
    h = euclidean_dist(pts[0], pts[3])
    if h == 0:
        return 0.3
    return (v1 + v2) / (2.0 * h)

def calculate_mar(landmarks, mouth_indices, width, height):
    """Calculates Mouth Aspect Ratio (MAR) for yawn detection."""
    pts = [(int(landmarks[idx].x * width), int(landmarks[idx].y * height)) for idx in mouth_indices]
    # Vertical distance between upper and lower lips
    v = euclidean_dist(pts[2], pts[3])
    # Horizontal distance between corner of mouth
    h = euclidean_dist(pts[0], pts[1])
    if h == 0:
        return 0.0
    return v / h

def main():
    try:
        import mediapipe as mp
    except ImportError:
        print("Please install required libraries: pip install mediapipe opencv-python requests")
        return

    mp_face_mesh = mp.solutions.face_mesh
    face_mesh = mp_face_mesh.FaceMesh(
        max_num_faces=1,
        refine_landmarks=True,
        min_detection_confidence=0.5,
        min_tracking_confidence=0.5
    )

    cap = cv2.VideoCapture(0)
    if not cap.isOpened():
        print("Error: Could not open camera.")
        return

    drowsy_frame_count = 0
    yawn_frame_count   = 0
    last_alert_time    = 0

    print("Driver AI Monitoring Started. Press 'q' to quit.")

    while True:
        ret, frame = cap.read()
        if not ret:
            break

        # Flip horizontally for natural mirror display
        frame = cv2.flip(frame, 1)
        h, w, _ = frame.shape
        rgb_frame = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
        results = face_mesh.process(rgb_frame)

        status_text = "Status: Alert & Attentive"
        status_color = (0, 255, 0)

        if results.multi_face_landmarks:
            landmarks = results.multi_face_landmarks[0].landmark

            # Calculate EAR and MAR
            left_ear  = calculate_ear(landmarks, LEFT_EYE_LANDMARKS, w, h)
            right_ear = calculate_ear(landmarks, RIGHT_EYE_LANDMARKS, w, h)
            ear       = (left_ear + right_ear) / 2.0
            mar       = calculate_mar(landmarks, MOUTH_LANDMARKS, w, h)

            # Check Drowsiness (Closed Eyes)
            if ear < EAR_THRESHOLD:
                drowsy_frame_count += 1
                if drowsy_frame_count >= EAR_CONSEC_FRAMES:
                    status_text = "⚠️ DROWSINESS DETECTED! WAKE UP!"
                    status_color = (0, 0, 255)
                    # Play sound or alert
                    if time.time() - last_alert_time > ALERT_COOLDOWN_SEC:
                        last_alert_time = time.time()
                        snapshot_path = "drowsy_event.jpg"
                        cv2.imwrite(snapshot_path, frame)
                        alert_msg = (
                            "🚨 *AI Vision Warning: Driver Drowsiness Detected!*\n\n"
                            f"• Eye Aspect Ratio: `{ear:.2f}` (Threshold: < {EAR_THRESHOLD})\n"
                            f"• Time: {time.strftime('%Y-%m-%d %H:%M:%S')}\n"
                            "Driver's eyes closed for consecutive frames!"
                        )
                        send_telegram_alert(alert_msg, snapshot_path)
            else:
                drowsy_frame_count = 0

            # Check Yawning (Early Fatigue Indicator)
            if mar > MAR_THRESHOLD:
                yawn_frame_count += 1
                if yawn_frame_count >= MAR_CONSEC_FRAMES:
                    status_text = "🥱 YAWNING DETECTED - REST RECOMMENDED"
                    status_color = (0, 165, 255)
                    if time.time() - last_alert_time > ALERT_COOLDOWN_SEC:
                        last_alert_time = time.time()
                        snapshot_path = "yawn_event.jpg"
                        cv2.imwrite(snapshot_path, frame)
                        alert_msg = (
                            "🥱 *Fatigue Warning: Yawning Detected!*\n\n"
                            f"• Mouth Aspect Ratio: `{mar:.2f}`\n"
                            "Driver shows signs of fatigue. Advised to take a break."
                        )
                        send_telegram_alert(alert_msg, snapshot_path)
            else:
                yawn_frame_count = 0

            # Display metrics on screen
            cv2.putText(frame, f"EAR: {ear:.2f}", (30, 40), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (255, 255, 255), 2)
            cv2.putText(frame, f"MAR: {mar:.2f}", (30, 70), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (255, 255, 255), 2)

        else:
            status_text = "No face detected in camera"
            status_color = (128, 128, 128)

        # Draw status bar
        cv2.putText(frame, status_text, (30, h - 30), cv2.FONT_HERSHEY_SIMPLEX, 0.8, status_color, 2)
        cv2.imshow("AI Driver Drowsiness & Fatigue Monitor", frame)

        if cv2.waitKey(1) & 0xFF == ord('q'):
            break

    cap.release()
    cv2.destroyAllWindows()

if __name__ == "__main__":
    main()
