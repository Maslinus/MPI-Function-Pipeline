#include <mpi.h>
#include <vector>
#include <iostream>
#include <cstdlib>

std::vector<int> f0(int count) {
    std::vector<int> numbers(count);
    for (int i = 0; i < count; ++i) {
        numbers[i] = (i % 2 == 0) ? 1 : 2;
    }
    return numbers;
}

std::vector<int> g0(int count) {
    std::vector<int> numbers(count);
    for (int i = 0; i < count; ++i) {
        numbers[i] = count - i;
    }
    return numbers;
}

std::vector<int> h0(int count) {
    std::vector<int> numbers(count);
    for (int i = 0; i < count; ++i) {
        numbers[i] = i+1;
    }
    return numbers;
}

int f1(int x) { 
    return x * x; 
}
int f2(int x) { 
    return x + x; 
}
int f3(int x) { 
    return x + 100;
}
int f4(int x) { 
    return x - 5;
}
int f5(int x) { 
    return x + 5.2; 
}

void last_f1(const std::vector<int>& result_array) {
    std::cout << "Result array: ";
    for (int i = 0; i < result_array.size(); ++i) {
        std::cout << result_array[i] << " ";
    }
    std::cout << std::endl;
}

void last_f2(const std::vector<int>& result_array) {
    std::cout << "last_f2 result (in reverse): ";
    for (int i = result_array.size() - 1; i >= 0; --i) {
        std::cout << result_array[i] << " ";
    }
    std::cout << std::endl;
}

void last_f3(const std::vector<int>& result_array) {
    std::cout << "last_f3 result (sum of elements): ";
    int sum = 0;
    for (int i = 0; i < result_array.size(); ++i) {
        sum += result_array[i];
    }
    std::cout << sum << std::endl;
}

int main(int argc, char** argv) { // mpiexec -n 4 MPIHelloWorld.exe
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    std::vector<int>(*start_function)(int) = h0;
    const int num_numbers = 10;
    const std::vector<int> processes_per_function = {1,1,1,1};
    void (*last_f)(const std::vector<int>&) = last_f1;

    const int num_functions = processes_per_function.size();
    const int last_rank = size - 1;

    int required_processes = 2;
    for (int procs : processes_per_function) {
        required_processes += procs;
    }

    if (size != required_processes) {
        if (rank == 0) {
            std::cerr << "Error: required " << required_processes << " processes." << std::endl;
        }
        MPI_Finalize();
        return 1;
    }

    const int MPI_TAG_END = 2;

    if (rank == 0) {
        std::vector<int> numbers = start_function(num_numbers);
        for (int i = 0; i < num_numbers; ++i) {
            int target_rank = 1 + (i % processes_per_function[0]);
            MPI_Send(&i, 1, MPI_INT, target_rank, 0, MPI_COMM_WORLD); // Индекс
            MPI_Send(&numbers[i], 1, MPI_INT, target_rank, 1, MPI_COMM_WORLD); // Число

            std::cout << "Process 0 sent number " << numbers[i] << " (index " << i << ") to process " << target_rank << std::endl;
        }
    }
    else if (rank == last_rank) {
        std::vector<int> results(num_numbers);
        for (int i = 0; i < num_numbers; ++i) {
            int number, index;
            MPI_Status status;

            std::cout << "Process " << rank << " is waiting to receive data." << std::endl;

            MPI_Recv(&index, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status); // Индекс

            MPI_Recv(&number, 1, MPI_INT, MPI_ANY_SOURCE, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE); // Результат

            results[index] = number;
            std::cout << "Process " << rank << " received result " << number << " (index " << index << ")" << std::endl;

        }

        last_f(results);
    }
    else if(rank>0 && rank<last_rank) {
        int current_rank_start = 1;

        for (int func_idx = 0; func_idx < num_functions; ++func_idx) {
            int num_procs = processes_per_function[func_idx];
            if (rank >= current_rank_start && rank < current_rank_start + num_procs) {
                for (int i = 0; i < num_numbers; ++i) {
                    int number, index;
                    MPI_Status status;
                    std::cout << "Process " << rank << " is waiting to receive data." << std::endl;

                    MPI_Recv(&index, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status); // Индекс


                    MPI_Recv(&number, 1, MPI_INT, MPI_ANY_SOURCE, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE); // Число

                    std::cout << "Process " << rank << " received number " << number << " (index " << index << ")" << std::endl;

                    int result;
                    if (func_idx == 0) {
                        result = f1(number);
                    }
                    else if (func_idx == 1) {
                        result = f2(number); 
                    }
                    else if (func_idx == 2) {
                        result = f3(number);
                    }
                    else if (func_idx == 3) {
                        result = f4(number);
                    }

                    if (func_idx < num_functions - 1) {
                        int next_rank_start = current_rank_start + num_procs;
                        int target_rank = next_rank_start + (i % processes_per_function[func_idx + 1]);

                        MPI_Send(&index, 1, MPI_INT, target_rank, 0, MPI_COMM_WORLD); // Индекс
                        MPI_Send(&result, 1, MPI_INT, target_rank, 1, MPI_COMM_WORLD); // Результат

                        std::cout << "Process " << rank << " sent result " << result << " (index " << index << ") to process " << target_rank << std::endl;
                    }
                    else if (func_idx == num_functions - 1) {
                        MPI_Send(&index, 1, MPI_INT, last_rank, 0, MPI_COMM_WORLD); // Индекс
                        MPI_Send(&result, 1, MPI_INT, last_rank, 1, MPI_COMM_WORLD); // Результат

                        std::cout << "Process " << rank << " sent final result " << result << " (index " << index << ") to process " << last_rank << std::endl;
                    }
                }
            }
            current_rank_start += num_procs;
        }
    }

    if (rank == 0) {
        std::cout << "PROCESS 0 IS ABOT TO SENT TERMINATION SIGNALS." << std::endl;

        for (int proc = 1; proc < size; ++proc) {
            int stop_signal = -1;
            MPI_Send(&stop_signal, 1, MPI_INT, proc, 2, MPI_COMM_WORLD); // Тег 2
            std::cout << "-> Sent termination signal to process " << proc << std::endl;
        }
        std::cout << "PROCESS 0 has finished." << std::endl;
    }
    else {
        bool running = true;

        while (running) {
            int number, index;
            MPI_Status status;

            MPI_Recv(&index, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

            if (status.MPI_TAG == 2 && index == -1) {
                std::cout << "!!! Process " << rank << " received termination signal. Exiting." << std::endl;
                running = false;
                continue;
            }
        }
    }

    MPI_Finalize();
    return 0;
}

//#include <mpi.h>
//#include <iostream>
//#include <vector>
//#include <functional>
//#include <memory>
//
//std::vector<int> f0(int count) {
//    std::vector<int> numbers(count);
//    for (int i = 0; i < count; ++i) {
//        numbers[i] = (i % 2 == 0) ? 1 : 2;
//    }
//    return numbers;
//}
//
//std::vector<int> g0(int count) {
//    std::vector<int> numbers(count);
//    for (int i = 0; i < count; ++i) {
//        numbers[i] = count - i;
//    }
//    return numbers;
//}
//
//std::vector<int> h0(int count) {
//    std::vector<int> numbers(count);
//    for (int i = 0; i < count; ++i) {
//        numbers[i] = (1);
//    }
//    return numbers;
//}
//
//int f1(int x) { return x * x; }
//int f2(int x) { return x + x; }
//int f3(int x) { return x + 100; }
//int f4(int x) { return x - 5; }
//int f5(int x) { return x + 5.2; }
//
//void last_f1(const std::vector<int>& result_array) {
//    std::cout << "Result array: ";
//    for (int i = 0; i < result_array.size(); ++i) {
//        std::cout << result_array[i] << " ";
//    }
//    std::cout << std::endl;
//}
//
//void last_f2(const std::vector<int>& result_array) {
//    std::cout << "last_f2 result (in reverse): ";
//    for (int i = result_array.size() - 1; i >= 0; --i) {
//        std::cout << result_array[i] << " ";
//    }
//    std::cout << std::endl;
//}
//
//void last_f3(const std::vector<int>& result_array) {
//    std::cout << "last_f3 result (sum of elements): ";
//    int sum = 0;
//    for (int i = 0; i < result_array.size(); ++i) {
//        sum += result_array[i];
//    }
//    std::cout << sum << std::endl;
//}
//
//int main(int argc, char** argv) {
//    MPI_Init(&argc, &argv);
//
//    int rank, size;
//    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
//    MPI_Comm_size(MPI_COMM_WORLD, &size);
//
//    std::vector<int>(*start_function)(int) = f0;
//    std::vector<std::function<int(int)>> functions = { f1, f2, f3, f4, f5 };
//    std::vector<int> processes_per_function = { 3, 2, 1, 1, 1 };
//    int num_functions = functions.size();
//    void (*last_f)(const std::vector<int>&) = last_f1;
//
//    int required_processes = 1; // Начальный процесс
//    for (int procs : processes_per_function) {
//        required_processes += procs;
//    }
//    required_processes += 1; // Финальный процесс
//
//    if (size != required_processes) {
//        if (rank == 0) {
//            std::cerr << "Error: required " << required_processes << " processes." << std::endl;
//        }
//        MPI_Finalize();
//        return 1;
//    }
//
//    const int num_numbers = 10;
//    std::vector<int> result_array(num_numbers);
//
//    if (rank == 0) {
//        std::vector<int> numbers = start_function(num_numbers);
//        for (int i = 0; i < num_numbers; ++i) {
//            int target_rank = (i % (size - 1)) + 1;
//            MPI_Send(&numbers[i], 1, MPI_INT, target_rank, 0, MPI_COMM_WORLD);
//        }
//    }
//    else {
//        int prev_rank = 0;
//        int current_rank = 1;
//
//        for (int func_idx = 0; func_idx < num_functions; ++func_idx) {
//            int num_procs = processes_per_function[func_idx];
//            if (rank >= current_rank && rank < current_rank + num_procs) {
//                for (int i = 0; i < num_numbers; ++i) {
//                    int number;
//                    MPI_Recv(&number, 1, MPI_INT, prev_rank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
//                    int result = functions[func_idx](number);
//                    MPI_Send(&result, 1, MPI_INT, current_rank + num_procs, 0, MPI_COMM_WORLD);
//                }
//            }
//            prev_rank = current_rank;
//            current_rank += num_procs;
//        }
//    }
//
//    if (rank == size - 1) {
//        for (int i = 0; i < num_numbers; ++i) {
//            MPI_Recv(&result_array[i], 1, MPI_INT, size - 2, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
//        }
//        last_f(result_array);
//    }
//
//    MPI_Finalize();
//    return 0;
//}