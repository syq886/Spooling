#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// 进程控制块
struct pcb {
    int id;        // 进程标识数
    int status;    // 进程状态
    int count;     // 要输出的文件数
    int x;         // 进程输出时的临时变量
    int filelength;// 进程的某个文件的长度
    int privateBuffer[200]; // 输出块不足缓冲区
    int privateBufferIndex; // 输出块不足缓冲区索引
    int privateBuffer2[200]; // 输入井不足缓冲区
    int privateBufferIndex2; // 输入井不足缓冲区索引
};

// 请求输出块
struct reqblock {
    int reqname;   // 请求进程名
    int length;    // 本次输出信息长度
    int addr;      // 信息在输出井的首地址
};

int buffer[2][100];

pcb PCB[3];

struct reqblock reqblock[10];

int C1[2] = { 100, 100 };

int C2[2][2] = { {0, 0}, {0, 0} };

int C3 = 10;

int Ptr1 = 0;
int Ptr2 = 0;

//已经被申请的请求块
int reqblockCount = 0;

void change_status(int pid, int new_status) {
    PCB[pid].status = new_status;
}

// 随机数生成函数，返回 [0, 1] 区间的随机数
double random() {
    return (double)rand() / RAND_MAX;
}
bool isreqBlockFull() {
    // 检查输出块数组是否已满
    return reqblockCount == 10;
}

bool isreqBlockEmpty() {
    // 检查输出块数组是否为空
    return reqblockCount == 0;
}

// 输出服务程序
void output_service(int pid, int x) {
    // 检查输出井空间是否足够
    if (C1[pid] == 0) {
        // 输出井满，将随机数存入进程的输入井缓冲区
        printf("用户进程%d在将输出信息送输出井时，输出井已满,阻塞，此时阻塞未输入的数据是：", pid);
        change_status(pid, 1);
        printf("进程%d：%d", pid, x);
        PCB[pid].privateBuffer2[PCB[pid].privateBufferIndex2++] = x;
        printf(" 转进程调度\n");
        return;
    }

    // 输出信息到输出井中
    buffer[pid][C2[pid][0]] = x;
    C1[pid]--;
    C2[pid][0] = (C2[pid][0] + 1) % 100;

    if (x != 0) {
        printf("用户进程%d输出%d\n", pid, x);
        PCB[pid].filelength++;
    }
    // 形成请求输出块
    else if (x == 0 && reqblockCount < 10) {
        printf("用户进程%d输出0,形成一个文件\n", pid);
        reqblock[Ptr2].reqname = pid;
        reqblock[Ptr2].length = ++PCB[pid].filelength;// 计算输出信息长度
        reqblock[Ptr2].addr = (C2[pid][0] - PCB[pid].filelength + 100) % 100; // 设置输出信息首地址
        Ptr2 = (Ptr2 + 1) % 10;
        C3--;
        reqblockCount++;
        PCB[pid].count--;
        printf("进程 %d 形成输出请求块，长度:%d，首地址:%d\n", pid, PCB[pid].filelength, (C2[pid][0] - PCB[pid].filelength + 100) % 100);
        puts("+------------------------------------------------------------+");
        PCB[pid].filelength = 0;
    }
    //无请求输出块，将随机数存入进程的输出块缓冲区
    else if (x == 0 && reqblockCount == 10) {
        printf("用户进程%d输出0\n", pid);
        printf("用户进程%d申请请求输出块时，没有可用请求块,阻塞,未形成文件，此时阻塞的数据有：\n", pid);
        change_status(pid, 3);
        PCB[pid].filelength = PCB[pid].filelength + 1;
        C2[pid][0] = (C2[pid][0] - PCB[pid].filelength + 100) % 100;
        for (int i = 0; i < PCB[pid].filelength; i++) {
            PCB[pid].privateBuffer[PCB[pid].privateBufferIndex++] = buffer[pid][C2[pid][0]];
            printf("%d ", buffer[pid][C2[pid][0]]);
            C2[pid][0] = (C2[pid][0] + 1) % 100;
        }
        C2[pid][0] = (C2[pid][0] - PCB[pid].filelength + 100) % 100;
        PCB[pid].filelength = 0;
        printf("转进程调度\n");
        return;
    }
    // 唤醒 SP00LING 进程
    if (PCB[2].status == 2) {
        change_status(2, 0);
    }
}

void spooling() {
    // 请求输出块是否为空
    if (isreqBlockEmpty()) {
        if (PCB[0].status == 4 && PCB[1].status == 4) {
            // SP00LING 进程结束
            change_status(2, 4);
            printf("------------Spooling进程结束------------\n");
            printf("两请求输出的进程结束，Spooling进程结束，程序结束\n");
            exit(0);
        }
        else {
            // SP00LING 进程等待
            change_status(2, 2);
            printf("无输出块，Spooling进程等待，转进程调度\n");
            return;
        }
    }
    else {
        while (C3 < 10) {
            //将输入井中的信息输出
            printf("start spooling,此时输出的是进程%d的文件信息:\n", reqblock[Ptr1].reqname);
            int pid = reqblock[Ptr1].reqname;
            int length = reqblock[Ptr1].length;
            int addr = reqblock[Ptr1].addr;

            //释放输出井
            for (int i = 0; i < length; i++) {
                printf(" %d ", buffer[pid][(addr + i) % 100]);
                C1[pid]++;  
            }
            printf("\n");
            printf("\n");

            // 释放输出块
            reqblockCount--;
            Ptr1 = (Ptr1 + 1) % 10;
            C3++;

            // 是否有等待输出井空的进程
            if (PCB[0].status == 1 || PCB[1].status == 1) {
                // 唤醒相应的等待进程
                if (PCB[0].status == 1 && pid == 0) {
                    change_status(0, 0);
                    printf("进程0等待输出井的缓冲区数据再次输入：\n");
                    for (int i = 0; i < PCB[0].privateBufferIndex2; i++) {
                        output_service(0, PCB[0].privateBuffer2[i]);
                    }
                    PCB[0].privateBufferIndex2 = 0; // 重置缓冲区索引
                    printf("进程0输入井缓冲区数据输入结束！\n");
                    return;
                }
                if (PCB[1].status == 1 && pid == 1) {
                    change_status(1, 0);
                    printf("进程1等待输出井的缓冲区数据再次输入：\n");
                    for (int i = 0; i < PCB[1].privateBufferIndex2; i++) {
                        output_service(1, PCB[1].privateBuffer2[i]);
                    }
                    PCB[1].privateBufferIndex2 = 0; // 重置缓冲区索引
                    printf("进程1输入井缓冲区数据输入结束！\n");
                    return;
                }
            }

            // 是否有等待输出块的进程
            if (PCB[0].status == 3 || PCB[1].status == 3) {
                // 唤醒相应的等待进程
                if (PCB[0].status == 3) {
                    change_status(0, 0);
                    printf("进程0等待请求输出块的缓冲区数据再次输出：\n");
                    for (int i = 0; i < PCB[0].privateBufferIndex; i++) {
                        output_service(0, PCB[0].privateBuffer[i]);
                    }
                    PCB[0].privateBufferIndex = 0;
                    printf("进程0请求输出块缓冲区数据输出结束！\n");
                    return;
                }
                if (PCB[1].status == 3) {
                    change_status(1, 0);
                    printf("进程1等待请求输出块的缓冲区数据再次输出：\n");
                    for (int i = 0; i < PCB[1].privateBufferIndex; i++) {
                        output_service(1, PCB[1].privateBuffer[i]);
                    }
                    PCB[1].privateBufferIndex = 0; // 重置缓冲区索引
                    printf("进程1请求输出块缓冲区数据输出结束！\n");
                    return;
                }
            }
        }
    }

}

// 用户进程
void user_process(int pid) {
    while (PCB[pid].count > 0) {
        //用户进程输出信息
        PCB[pid].x = random() * 10;

        // 调用输出服务程序，将输出信息送入输出井，并形成请求输出块
        output_service(pid, PCB[pid].x);

        // 输出井已满，等待
        if (PCB[pid].status == 1) {
            return;
        }

        // 请求输出块已满，等待
        if (PCB[pid].status == 3) {
           // printf("用户进程%d申请请求输出块时，没有可用请求块,阻塞\n", pid);
            return;
        }
    }

    // 所有文件输出完毕，进程结束
    printf("--------进程%d的文件已经全部输入完毕，进程结束---------", pid);
    change_status(pid, 4);
    printf("\n");
}

// 进程调度函数
void decideprocess() {
    double x = random();
    if (x > 0.9 && PCB[2].status == 0) {
        printf("\n");
        spooling();
    }
    else if (x <= 0.9 && x > 0.45 && PCB[1].status == 0) {
        printf("\n");
        printf("start userprocess1：\n");
        user_process(1);
    }
    else if (x <= 0.45 && PCB[0].status == 0) {
        printf("\n");
        printf("start userprocess0：\n");
        user_process(0);
    }
}

int main() {
  
    srand(time(NULL));

    printf("请输入两个文件的个数:");
    scanf_s("%d%d", &PCB[0].count, &PCB[1].count);
    PCB[0].id = 0;
    PCB[0].filelength = 0;
    PCB[0].status = 0;
    PCB[0].privateBufferIndex = 0;
    PCB[0].privateBufferIndex2 = 0;

    PCB[1].id = 1;
    PCB[1].filelength = 0;
    PCB[1].status = 0;
    PCB[1].privateBufferIndex = 0;
    PCB[0].privateBufferIndex2 = 0;

    // 循环调度进程
    while (true) {
        decideprocess();
    }
    return 0;
}
