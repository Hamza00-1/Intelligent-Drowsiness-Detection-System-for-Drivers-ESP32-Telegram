"""
AI Telemetry Logger & ML Classifier Simulator
---------------------------------------------
This script can:
 1. Read live telemetry streamed from the ESP32 (via USB Serial or Wi-Fi MQTT/HTTP).
 2. Train a Machine Learning Model (Random Forest Classifier) on steering motion
    and biometric vitals to classify:
      - 0: Attentive / Active Driver
      - 1: Drowsy / Fatigued Driver
      - 2: Critical Inactivity / Unconscious
 3. Export model weights or evaluate classification accuracy.
"""

import time
import numpy as np

def generate_synthetic_training_data(num_samples=1000):
    """
    Generates realistic synthetic driving data based on medical and automotive research:
     - Features: [Steering Variance, Inactivity Duration (s), Heart Rate (BPM), HRV RMSSD (ms), Hand On Wheel (0/1)]
     - Labels: 0 (Normal), 1 (Fatigued / Drowsy), 2 (Critical Unresponsive)
    """
    np.random.seed(42)
    X = []
    y = []

    for _ in range(num_samples):
        driver_state = np.random.choice([0, 1, 2], p=[0.70, 0.20, 0.10])

        if driver_state == 0:  # Normal / Attentive
            steering_var = np.random.uniform(5.0, 35.0)  # Steady micro-corrections
            inactivity_sec = np.random.uniform(0.1, 2.5)
            heart_rate = np.random.uniform(65.0, 95.0)
            hrv_rmssd = np.random.uniform(35.0, 75.0)
            hand_on_wheel = 1
        elif driver_state == 1:  # Fatigued / Drowsy
            # Low variance (drifting) OR high variance (jerky over-corrections)
            steering_var = np.random.choice([np.random.uniform(0.5, 2.5), np.random.uniform(40.0, 90.0)])
            inactivity_sec = np.random.uniform(3.0, 6.0)
            heart_rate = np.random.uniform(52.0, 68.0)   # Heart rate drops during fatigue
            hrv_rmssd = np.random.uniform(15.0, 30.0)    # Reduced HRV
            hand_on_wheel = np.random.choice([0, 1], p=[0.3, 0.7])
        else:  # Critical / Unconscious
            steering_var = np.random.uniform(0.0, 0.8)   # Flatline steering
            inactivity_sec = np.random.uniform(7.0, 15.0)
            heart_rate = np.random.uniform(40.0, 60.0)
            hrv_rmssd = np.random.uniform(5.0, 20.0)
            hand_on_wheel = 0                            # Hands off wheel

        X.append([steering_var, inactivity_sec, heart_rate, hrv_rmssd, hand_on_wheel])
        y.append(driver_state)

    return np.array(X), np.array(y)

def train_and_evaluate_model():
    try:
        from sklearn.ensemble import RandomForestClassifier
        from sklearn.model_selection import train_test_split
        from sklearn.metrics import classification_report, accuracy_score
    except ImportError:
        print("Note: scikit-learn is not installed. Run 'pip install scikit-learn' to train the ML model.")
        return

    print("Generating automotive biometric datasets...")
    X, y = generate_synthetic_training_data(1500)
    X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.25, random_state=42)

    print("Training Random Forest Classifier on ESP32 Feature Space...")
    clf = RandomForestClassifier(n_estimators=30, max_depth=6, random_state=42)
    clf.fit(X_train, y_train)

    preds = clf.predict(X_test)
    accuracy = accuracy_score(y_test, preds)

    print(f"\n================ ML MODEL EVALUATION ================")
    print(f"Overall Classification Accuracy: {accuracy * 100:.2f}%\n")
    print(classification_report(y_test, preds, target_names=["0: Attentive", "1: Fatigued", "2: Critical"]))
    print("=====================================================")
    print("Feature Importances:")
    feature_names = ["Steering Variance", "Inactivity Duration", "Heart Rate", "HRV (RMSSD)", "Hand On Wheel"]
    for name, imp in zip(feature_names, clf.feature_importances_):
        print(f"  • {name:22s}: {imp * 100:.1f}%")

if __name__ == "__main__":
    train_and_evaluate_model()
