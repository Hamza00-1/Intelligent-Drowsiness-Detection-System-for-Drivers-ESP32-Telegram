"""
Enregistreur de Télémétrie IA & Simulateur de Modèle Machine Learning
---------------------------------------------------------------------
Ce script permet de :
 1. Lire la télémétrie en direct émise par l'ESP32.
 2. Entraîner un modèle de Machine Learning (Random Forest Classifier)
    sur les mouvements du volant et les constantes biométriques :
      - 0: Conducteur Attentif / Actif
      - 1: Conducteur Somnolent / Fatigué
      - 2: Inactivité Critique / Perte de Conscience
 3. Évaluer la précision de classification et les importances de variables.
"""

import time
import numpy as np

def generate_synthetic_training_data(num_samples=1000):
    """
    Génère des données synthétiques réalistes basées sur la recherche automobile et médicale :
     - Variables : [Variance Volant, Durée d'Inactivité (s), Pouls (BPM), VRC RMSSD (ms), Main sur le Volant (0/1)]
     - Classes : 0 (Normal), 1 (Fatigué / Somnolent), 2 (Critique / Sans réaction)
    """
    np.random.seed(42)
    X = []
    y = []

    for _ in range(num_samples):
        driver_state = np.random.choice([0, 1, 2], p=[0.70, 0.20, 0.10])

        if driver_state == 0:  # Attentif
            steering_var = np.random.uniform(5.0, 35.0)  # Micro-ajustements réguliers
            inactivity_sec = np.random.uniform(0.1, 2.5)
            heart_rate = np.random.uniform(65.0, 95.0)
            hrv_rmssd = np.random.uniform(35.0, 75.0)
            hand_on_wheel = 1
        elif driver_state == 1:  # Somnolent / Fatigué
            steering_var = np.random.choice([np.random.uniform(0.5, 2.5), np.random.uniform(40.0, 90.0)])
            inactivity_sec = np.random.uniform(3.0, 6.0)
            heart_rate = np.random.uniform(52.0, 68.0)
            hrv_rmssd = np.random.uniform(15.0, 30.0)
            hand_on_wheel = np.random.choice([0, 1], p=[0.3, 0.7])
        else:  # Critique / Inconscient
            steering_var = np.random.uniform(0.0, 0.8)   # Direction figée
            inactivity_sec = np.random.uniform(7.0, 15.0)
            heart_rate = np.random.uniform(40.0, 60.0)
            hrv_rmssd = np.random.uniform(5.0, 20.0)
            hand_on_wheel = 0                            # Mains hors du volant

        X.append([steering_var, inactivity_sec, heart_rate, hrv_rmssd, hand_on_wheel])
        y.append(driver_state)

    return np.array(X), np.array(y)

def train_and_evaluate_model():
    try:
        from sklearn.ensemble import RandomForestClassifier
        from sklearn.model_selection import train_test_split
        from sklearn.metrics import classification_report, accuracy_score
    except ImportError:
        print("Remarque : scikit-learn n'est pas installé. Exécutez 'pip install scikit-learn' pour entraîner le modèle.")
        return

    print("Génération du jeu de données biométriques et cinématiques...")
    X, y = generate_synthetic_training_data(1500)
    X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.25, random_state=42)

    print("Entraînement de la forêt d'arbres décisionnels (Random Forest Classifier)...")
    clf = RandomForestClassifier(n_estimators=30, max_depth=6, random_state=42)
    clf.fit(X_train, y_train)

    preds = clf.predict(X_test)
    accuracy = accuracy_score(y_test, preds)

    print(f"\n================ ÉVALUATION DU MODÈLE DE MACHINE LEARNING ================")
    print(f"Précision Globale de Classification : {accuracy * 100:.2f}%\n")
    print(classification_report(y_test, preds, target_names=["0: Attentif", "1: Somnolent", "2: Critique"]))
    print("=========================================================================")
    print("Importance Relative des Variables :")
    feature_names = ["Variance du Volant", "Durée d'Inactivité", "Rythme Cardiaque", "VRC (RMSSD)", "Mains sur le Volant"]
    for name, imp in zip(feature_names, clf.feature_importances_):
        print(f"  • {name:25s}: {imp * 100:.1f}%")

if __name__ == "__main__":
    train_and_evaluate_model()
