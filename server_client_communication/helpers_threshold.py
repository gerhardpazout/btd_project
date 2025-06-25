
import pandas as pd
import numpy as np
from pathlib import Path
import pandas as pd

"""
This file contains all the helper functions used for calculating the thresholds that are then sent to the ESP / client
"""

def calculate_movement_magnitude(df):
    df['movement_mag'] = np.sqrt(df['x']**2 + df['y']**2 + df['z']**2)
    return df

def create_analysis_windows(df, window_size_seconds):
    duration_sec = (df['timestamp'].iloc[-1] - df['timestamp'].iloc[0]) / 1000
    sampling_rate = len(df) / duration_sec
    samples_per_window = int(sampling_rate * window_size_seconds)

    windows = []
    for i in range(0, len(df) - samples_per_window, samples_per_window):
        window_data = df.iloc[i:i + samples_per_window]
        if len(window_data) >= samples_per_window * 0.9:
            movement_variance = np.var(window_data['movement_mag'].values)
            temps = window_data['temp'].values
            temp_trend = temps[-1] - temps[0]
            time_start = window_data['timestamp'].iloc[0]
            windows.append({
                'start_timestamp': time_start,
                'movement_variance': movement_variance,
                'temp_trend': temp_trend,
                'avg_temp': np.mean(temps)
            })

    return pd.DataFrame(windows)

def calculate_thresholds(df: pd.DataFrame) -> tuple[float, float]:
    df = df.dropna()
    df = df[(df['x'].abs() < 50) & (df['y'].abs() < 50) & (df['z'].abs() < 50) &
            (df['temp'] > 0) & (df['temp'] < 100)]

    df['movement_mag'] = np.sqrt(df['x']**2 + df['y']**2 + df['z']**2)

    variances = df['movement_mag'].rolling(window=30, min_periods=10).var().dropna()

    low_thresh = np.percentile(variances, 25)
    high_thresh = np.percentile(variances, 75)

    return low_thresh, high_thresh

def merge_csvs_from_directory(directory_path):
    """Merge all CSV files in the given directory into a single DataFrame."""
    directory = Path(directory_path)
    all_csvs = list(directory.glob("*.csv"))

    df_list = []
    for csv_file in all_csvs:
        df = pd.read_csv(csv_file)
        df_list.append(df)

    if df_list:
        merged_df = pd.concat(df_list, ignore_index=True)
        return merged_df
    else:
        return pd.DataFrame()  # Empty if no CSVs found