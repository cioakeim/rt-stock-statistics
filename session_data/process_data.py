import numpy as np
import matplotlib.pyplot as plt
import os
import glob


def produce_data_stats(folder_path):
    # Get file batch
    csv_files=glob.glob(os.path.join(folder_path,'*.csv'))
    # For each file
    for file in csv_files:
        basename=os.path.basename(file)
        # Open whole file
        data=np.genfromtxt(file,delimiter=',',skip_header=0)
        # And keep delays only
        print(f"For {basename}:\nMean delay(us): {np.mean(data)}")
        print(f"Standard deviation(us): {np.std(data)}")
        print(f"Max(us): {np.max(data)}\nMin(us): {np.min(data)}")
        
        create_vector_histogram(data,basename)
        plt.figure()
        plt.plot(data)
        plt.xlabel("Work item number")
        plt.ylabel("Delay of work item (us)")
        pure_name=basename.removesuffix('.csv')
        pure_name=pure_name.capitalize()
        pure_name=pure_name.replace("_"," ")
        plt.title(f"{pure_name}: Delay sequence")
    plt.show()
    return

def create_vector_histogram(vector,name):
    plt.figure()
    max_bin=np.percentile(vector,99)
    plt.hist(vector,bins=30,range=(np.min(vector),max_bin),edgecolor='black',
             density=True)
    plt.xlabel("Delay (us)")
    plt.ylabel("Density (%)")
    pure_name=name.removesuffix('.csv')
    pure_name=pure_name.capitalize()
    pure_name=pure_name.replace("_"," ")
    plt.title(f"{pure_name}: Histogram")

produce_data_stats('./delays')
