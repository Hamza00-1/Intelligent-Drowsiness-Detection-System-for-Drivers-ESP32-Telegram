"""
Module d'IA pour la Détection de Somnolence et de Bâillements
------------------------------------------------------------
Utilise la Vision par Ordinateur et le Deep Learning (MediaPipe Face Mesh + OpenCV)
pour surveiller la vigilance du conducteur en temps réel :
 1. Ratio d'Aspect de l'Œil (EAR) - Détecte les yeux fermés et les micro-sommeils.
 2. Ratio d'Aspect de la Bouche (MAR) - Détecte les bâillements répétés (fatigue précoce).
 3. Intégration Telegram Bot - Envoie des alertes instantanées avec photo capturée.

Prérequis :
  pip install opencv-python mediapipe requests numpy
"""

import time
import math
import cv2
import numpy as np
import requests

# ======================== CONFIGURATION ========================
TELEGRAM_BOT_TOKEN = "VOTRE_TELEGRAM_BOT_TOKEN"
TELEGRAM_CHAT_ID   = "VOTRE_TELEGRAM_CHAT_ID"

# Seuils de Sécurité
EAR_THRESHOLD         = 0.21    # En-dessous de ce seuil : l'œil est fermé
EAR_CONSEC_FRAMES     = 30      # ~1.0 à 1.5 seconde à 30 FPS
MAR_THRESHOLD         = 0.65    # Au-dessus de ce seuil : la bouche est grande ouverte (bâillement)
MAR_CONSEC_FRAMES     = 35      # Durée soutenue d'un bâillement
ALERT_COOLDOWN_SEC    = 20      # Délai d'attente entre deux alertes Telegram

# Indices des Repères Faciaux MediaPipe (Landmarks)
LEFT_EYE_LANDMARKS  = [362, 385, 387, 263, 373, 380]
RIGHT_EYE_LANDMARKS = [33,  160, 158, 133, 153, 144]
MOUTH_LANDMARKS     = [61, 291, 13, 14, 78, 308]
# ===============================================================

def send_telegram_alert(message, image_path=None):
    """Transmet un message et une photo de capture à Telegram."""
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
        print("Alerte Telegram envoyée avec succès ! Code de réponse :", response.status_code)
    except Exception as e:
        print(f"Échec de l'envoi de l'alerte Telegram : {e}")

def euclidean_dist(pt1, pt2):
    return math.hypot(pt1[0] - pt2[0], pt1[1] - pt2[1])

def calculate_ear(landmarks, eye_indices, width, height):
    """Calcule le ratio d'aspect de l'œil (EAR)."""
    pts = [(int(landmarks[idx].x * width), int(landmarks[idx].y * height)) for idx in eye_indices]
    v1 = euclidean_dist(pts[1], pts[5])
    v2 = euclidean_dist(pts[2], pts[4])
    h = euclidean_dist(pts[0], pts[3])
    if h == 0:
        return 0.3
    return (v1 + v2) / (2.0 * h)

def calculate_mar(landmarks, mouth_indices, width, height):
    """Calcule le ratio d'aspect de la bouche (MAR) pour les bâillements."""
    pts = [(int(landmarks[idx].x * width), int(landmarks[idx].y * height)) for idx in mouth_indices]
    v = euclidean_dist(pts[2], pts[3])
    h = euclidean_dist(pts[0], pts[1])
    if h == 0:
        return 0.0
    return v / h

def main():
    try:
        import mediapipe as mp
    except ImportError:
        print("Veuillez installer les bibliothèques requises : pip install mediapipe opencv-python requests")
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
        print("Erreur : Impossible d'accéder à la webcam.")
        return

    drowsy_frame_count = 0
    yawn_frame_count   = 0
    last_alert_time    = 0

    print("Surveillance IA du conducteur démarrée. Appuyez sur 'q' pour quitter.")

    while True:
        ret, frame = cap.read()
        if not ret:
            break

        frame = cv2.flip(frame, 1)
        h, w, _ = frame.shape
        rgb_frame = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
        results = face_mesh.process(rgb_frame)

        status_text = "Statut: Conducteur Attentif"
        status_color = (0, 255, 0)

        if results.multi_face_landmarks:
            landmarks = results.multi_face_landmarks[0].landmark

            left_ear  = calculate_ear(landmarks, LEFT_EYE_LANDMARKS, w, h)
            right_ear = calculate_ear(landmarks, RIGHT_EYE_LANDMARKS, w, h)
            ear       = (left_ear + right_ear) / 2.0
            mar       = calculate_mar(landmarks, MOUTH_LANDMARKS, w, h)

            # Détection de la somnolence (Yeux fermés prolongés)
            if ear < EAR_THRESHOLD:
                drowsy_frame_count += 1
                if drowsy_frame_count >= EAR_CONSEC_FRAMES:
                    status_text = "⚠️ SOMNOLENCE DÉTECTÉE ! RÉVEILLEZ-VOUS !"
                    status_color = (0, 0, 255)
                    if time.time() - last_alert_time > ALERT_COOLDOWN_SEC:
                        last_alert_time = time.time()
                        snapshot_path = "capture_somnolence.jpg"
                        cv2.imwrite(snapshot_path, frame)
                        alert_msg = (
                            "🚨 *Alerte Vision IA : Somnolence au Volant Détectée !*\n\n"
                            f"• Ratio d'ouverture oculaire (EAR) : `{ear:.2f}` (Seuil : < {EAR_THRESHOLD})\n"
                            f"• Horodatage : {time.strftime('%Y-%m-%d %H:%M:%S')}\n"
                            "Les yeux du conducteur sont restés fermés de manière critique !"
                        )
                        send_telegram_alert(alert_msg, snapshot_path)
            else:
                drowsy_frame_count = 0

            # Détection des bâillements (Fatigue précoce)
            if mar > MAR_THRESHOLD:
                yawn_frame_count += 1
                if yawn_frame_count >= MAR_CONSEC_FRAMES:
                    status_text = "🥱 BÂILLEMENT DÉTECTÉ - PAUSE RECOMMANDÉE"
                    status_color = (0, 165, 255)
                    if time.time() - last_alert_time > ALERT_COOLDOWN_SEC:
                        last_alert_time = time.time()
                        snapshot_path = "capture_baillement.jpg"
                        cv2.imwrite(snapshot_path, frame)
                        alert_msg = (
                            "🥱 *Avertissement de Fatigue : Bâillement Détecté !*\n\n"
                            f"• Ratio d'ouverture de la bouche (MAR) : `{mar:.2f}`\n"
                            "Le conducteur montre des signes de fatigue. Une pause est fortement conseillée."
                        )
                        send_telegram_alert(alert_msg, snapshot_path)
            else:
                yawn_frame_count = 0

            cv2.putText(frame, f"EAR (Yeux): {ear:.2f}", (30, 40), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (255, 255, 255), 2)
            cv2.putText(frame, f"MAR (Bouche): {mar:.2f}", (30, 70), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (255, 255, 255), 2)

        else:
            status_text = "Aucun visage détecté face à la caméra"
            status_color = (128, 128, 128)

        cv2.putText(frame, status_text, (30, h - 30), cv2.FONT_HERSHEY_SIMPLEX, 0.8, status_color, 2)
        cv2.imshow("Moniteur IA de Somnolence et Fatigue", frame)

        if cv2.waitKey(1) & 0xFF == ord('q'):
            break

    cap.release()
    cv2.destroyAllWindows()

if __name__ == "__main__":
    main()
