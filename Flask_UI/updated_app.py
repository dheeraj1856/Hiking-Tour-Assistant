from flask import Flask, render_template
import csv

app = Flask(__name__)


def read_data_from_csv(filepath='data.csv'):
    """Reads the last line of the CSV file and returns a dictionary of the data."""
    try:
        with open(filepath, 'r') as file:
            # Assuming the CSV has headers and the last line contains the latest data
            reader = csv.DictReader(file)
            last_line = None
            for row in reader:
                last_line = row  # This will end with the last row

            if last_line is not None:
                # Convert appropriate fields to int or float as needed
                last_line['steps'] = float(last_line['steps'])
                last_line['calories'] = float(last_line['calories'])
                last_line['time'] = float(last_line['time'])
                last_line['distance'] = float(last_line['distance'])
                return last_line
            else:
                # Default values if the file is empty
                return {'steps': 0, 'calories': 0.0, 'time': 0.0, 'distance': 0.0}
    except FileNotFoundError:
        print("CSV file not found.")
        return {}
    except Exception as e:
        print(f"An error occurred: {e}")
        return {}


@app.route('/')
def index():
    data = read_data_from_csv()
    return render_template('index.html', data=data)


if __name__ == '__main__':
    app.run(debug=True)