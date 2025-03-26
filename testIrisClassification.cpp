// testIrisClassificationExperiments.cpp

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <algorithm>
#include <random>
#include <cmath>
#include "./src/logisticRegression.h" // Your previously implemented logistic regression from scratch.
#include "./src/polyReg.h"            // Provides genPolyFeatures(vector<vector<double>> X, int degree)
using std::cout;
using std::cerr;
using std::endl;
using std::vector;
using std::string;
using Matrix = std::vector<std::vector<double>>;

// Helper to compute classification accuracy.
double computeAccuracy(const vector<int>& y_true, const vector<int>& y_pred) {
    if (y_true.empty()) return 0.0;
    int correct = 0;
    for (size_t i = 0; i < y_true.size(); i++) {
        if (y_true[i] == y_pred[i])
            correct++;
    }
    return 100.0 * correct / y_true.size();
}

// Helper to compute 2x2 confusion matrix for binary classification.
// The matrix layout is:
// [ [TN, FP],
//   [FN, TP] ]
vector<vector<int>> confusionMatrix(const vector<int>& y_true, const vector<int>& y_pred) {
    vector<vector<int>> cm(2, vector<int>(2, 0));
    for (size_t i = 0; i < y_true.size(); i++) {
        if (y_true[i] == 0) {
            if (y_pred[i] == 0)
                cm[0][0]++;
            else
                cm[0][1]++;
        } else { // y_true[i] == 1
            if (y_pred[i] == 1)
                cm[1][1]++;
            else
                cm[1][0]++;
        }
    }
    return cm;
}

int main() {
    cout << "=== Iris Classification Experiment using Logistic Regression ===" << endl;
    
    // --- Load and preprocess the Iris dataset ---
    // File path (assumed to be in ./datasets/)
    string filename = "./datasets/Iris_Classification/Iris_Classification.csv";
    std::ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Error: Unable to open file " << filename << endl;
        return 1;
    }
    
    string line;
    // Read header
    std::getline(file, line);
    
    // We will use only samples from "Iris-versicolor" and "Iris-virginica"
    // The CSV format: Id,SepalLengthCm,SepalWidthCm,PetalLengthCm,PetalWidthCm,Species
    // We select as features the PetalLength and PetalWidth (columns index 3 and 4).
    Matrix X;
    vector<int> y;
    
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        std::istringstream iss(line);
        string token;
        vector<string> tokens;
        while (std::getline(iss, token, ',')) {
            tokens.push_back(token);
        }
        if (tokens.size() < 6) continue; // not enough fields
        
        // Get species (column 5)
        const string species = tokens[5];
        // Use only Iris-versicolor and Iris-virginica
        if (species == "Iris-versicolor" || species == "Iris-virginica") {
            try {
                double petalLength = std::stod(tokens[3]);
                double petalWidth  = std::stod(tokens[4]);
                // For features, we take only petal length and petal width.
                X.push_back({petalLength, petalWidth});
            } catch (...) {
                continue; // skip rows that do not parse properly
            }
            // Map species to numeric labels: let Iris-versicolor = 0, Iris-virginica = 1.
            if (species == "Iris-versicolor")
                y.push_back(0);
            else
                y.push_back(1);
        }
    }
    file.close();
    
    if (X.empty()) {
        cerr << "No valid data loaded from " << filename << endl;
        return 1;
    }
    
    // --- Shuffle and split the data into training and test sets ---
    vector<int> indices(X.size());
    for (size_t i = 0; i < X.size(); i++) {
        indices[i] = i;
    }
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(indices.begin(), indices.end(), g);
    
    Matrix X_shuffled;
    vector<int> y_shuffled;
    for (size_t i = 0; i < indices.size(); i++) {
        X_shuffled.push_back(X[indices[i]]);
        y_shuffled.push_back(y[indices[i]]);
    }
    const double train_ratio = 0.7;
    size_t train_size = static_cast<size_t>(X_shuffled.size() * train_ratio);
    
    Matrix X_train(X_shuffled.begin(), X_shuffled.begin() + train_size);
    vector<int> y_train(y_shuffled.begin(), y_shuffled.begin() + train_size);
    Matrix X_test(X_shuffled.begin() + train_size, X_shuffled.end());
    vector<int> y_test(y_shuffled.begin() + train_size, y_shuffled.end());
    
    // --- Apply a polynomial basis expansion ---
    // Here we use a degree 1 expansion (which simply adds a bias term)
    // It is assumed that genPolyFeatures() returns a matrix whose first column is ones.
    int degree = 1;  
    Matrix Phi_train = genPolyFeatures(X_train, degree);
    Matrix Phi_test  = genPolyFeatures(X_test, degree);
    
    // --- Train the logistic regression classifier ---
    LogisticRegression classifier(0.01, 10000, 1e-6);
    classifier.fit(Phi_train, y_train);
    
    // --- Evaluate on test data ---
    vector<int> predictions = classifier.predict(Phi_test);
    vector<double> probabilities = classifier.predictProb(Phi_test);
    double accuracy = computeAccuracy(y_test, predictions);
    cout << "Test Accuracy: " << accuracy << "%" << endl;
    
    vector<vector<int>> cm = confusionMatrix(y_test, predictions);
    
    // --- Write test predictions and probabilities to CSV ---
    {
        std::ofstream outFile("./results/iris_logistic_predictions.csv");
        outFile << "PetalLength,PetalWidth,true_label,predicted_label,probability\n";
        // Iterate over test set (note: these are the original features, not the expanded ones)
        for (size_t i = 0; i < X_test.size(); i++) {
            outFile << X_test[i][0] << "," << X_test[i][1] << ",";
            outFile << y_test[i] << "," << predictions[i] << "," << probabilities[i] << "\n";
        }
        outFile.close();
    }
    
    // --- Write confusion matrix to CSV ---
    {
        std::ofstream outFile("./results/iris_confusion_matrix.csv");
        outFile << " ,Predicted_0,Predicted_1\n";
        outFile << "True_0," << cm[0][0] << "," << cm[0][1] << "\n";
        outFile << "True_1," << cm[1][0] << "," << cm[1][1] << "\n";
        outFile.close();
    }
    
    // --- Decision Boundary Computation ---
    // To visualize the decision boundary, we sample a grid in the space spanned by PetalLength and PetalWidth.
    double minPL = 1e9, maxPL = -1e9, minPW = 1e9, maxPW = -1e9;
    // Use training set to define bounds.
    for (size_t i = 0; i < X_train.size(); i++) {
        if (X_train[i][0] < minPL) minPL = X_train[i][0];
        if (X_train[i][0] > maxPL) maxPL = X_train[i][0];
        if (X_train[i][1] < minPW) minPW = X_train[i][1];
        if (X_train[i][1] > maxPW) maxPW = X_train[i][1];
    }
    // Add small margins.
    double marginPL = 0.5;
    double marginPW = 0.5;
    minPL -= marginPL;  maxPL += marginPL;
    minPW -= marginPW;  maxPW += marginPW;
    
    // Generate grid points.
    const int gridSteps = 100;
    vector<vector<double>> gridPoints;
    double stepPL = (maxPL - minPL) / (gridSteps - 1);
    double stepPW = (maxPW - minPW) / (gridSteps - 1);
    for (int i = 0; i < gridSteps; i++) {
        for (int j = 0; j < gridSteps; j++) {
            double pl = minPL + i * stepPL;
            double pw = minPW + j * stepPW;
            gridPoints.push_back({pl, pw});
        }
    }
    // Apply same basis expansion to grid points.
    Matrix Phi_grid = genPolyFeatures(gridPoints, degree);
    vector<double> gridProbs = classifier.predictProb(Phi_grid);
    
    // Write decision boundary grid with predicted probabilities.
    {
        std::ofstream outFile("./results/iris_decision_boundary.csv");
        outFile << "PetalLength,PetalWidth,Probability\n";
        for (size_t i = 0; i < gridPoints.size(); i++) {
            outFile << gridPoints[i][0] << "," << gridPoints[i][1] << "," << gridProbs[i] << "\n";
        }
        outFile.close();
    }
    
    cout << "All results written to the ./results folder." << endl;
    
    return 0;
}