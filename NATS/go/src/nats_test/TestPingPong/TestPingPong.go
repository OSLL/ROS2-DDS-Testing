package TestPingPong

import(
	"strconv"
	"strings"
	"log"
	"os"
	"syscall"
	"sync"
	"time"
	"encoding/json"

	"github.com/nats-io/nats.go"
)
const(
	timeout = 20*time.Second
	watermark = 10
)

type info struct{
	Msg msg_info `json:"msg"`
}

type msg_info struct{
	Id int `json:"id"`
	Read_proc_time int64 `json:"read_proc_time"`
	Proc_time int64 `json:"proc_time"`
	Receive_timestamp int64 `json:"recieve_timestamp"`
	Sent_time int64 `json:"sent_time"`
	Delay int64 `json:"delay"`
}

type msg struct{
	Id int `json:"id"`
	Sent_time int64 `json:"sent_time"`
	Msg string `json:"msg"`
}

type TestPingPong struct {
	topic1 string
	topic2 string
	msgCount int
	prior int
	cpu_index int
	msgSize int
	interval int
	filename string
	topic_priority int
	isFirst bool
	isNew bool
	last_rec_msg_id *int
	msgSizeMin int
	msgSizeMax int
	step int
	msgs_before_step int
	nc *nats.Conn
	read_msg_time []int64
	write_msg_time []int64
	msgs [][]byte
	receive_timestamp []int64
	n_received *int
	rec_before *int
	mu sync.Mutex
}

func New(topic1 string, topic2 string, msgCount int, prior int, cpu_index int, msgSize int, interval int, filename string, topic_priority int, isFirst bool, msgSizeMin int, msgSizeMax int, step int, msgs_before_step int) TestPingPong{
	pid := os.Getpid()
	if prior >= 0 {
		err := syscall.Setpriority(syscall.PRIO_PROCESS, pid, prior)
		if err != nil {
			log.Fatal(err)
		}
	}
	if cpu_index >= 0 {
		err := os.MkdirAll("/sys/fs/cgroup/cpuset/sub_cpuset", os.ModePerm)
		if err != nil && err != os.ErrExist {
			log.Fatal(err)
		}
		f_cpu, err := os.OpenFile("/sys/fs/cgroup/cpuset/sub_cpuset/cpuset.cpus", os.O_WRONLY, 0577)
		if err != nil {
			log.Fatal(err)
		}
		defer f_cpu.Close()
		f_cpu.Write([]byte(strconv.Itoa(cpu_index)))

		f_exclusive, err := os.OpenFile("/sys/fs/cgroup/cpuset/sub_cpuset/cpuset.cpu_exclusive", os.O_WRONLY, 0577)
		if err != nil {
			log.Fatal(err)
		}
		defer f_exclusive.Close()
		f_exclusive.Write([]byte("1"))

		f_mem, err := os.OpenFile("/sys/fs/cgroup/cpuset/sub_cpuset/cpuset.mems", os.O_WRONLY, 0577)
		if err != nil {
			log.Fatal(err)
		}
		defer f_mem.Close()
		f_mem.Write([]byte("0"))

		f_task, err := os.OpenFile("/sys/fs/cgroup/cpuset/sub_cpuset/tasks", os.O_WRONLY, 0577)
		if err != nil {
			log.Fatal(err)
		}
		defer f_task.Close()
		f_task.Write([]byte(strconv.Itoa(pid)))
	}
	nc, err := nats.Connect(nats.DefaultURL)
        if err != nil {
                log.Fatal(err)
        }

	isNew := interval != 0
	if isNew {
		msgSize = msgSizeMax
	}

	pingpong := TestPingPong{topic1, topic2, msgCount, prior, cpu_index, msgSize, interval, filename, topic_priority, isFirst, isNew, new(int), msgSizeMin, msgSizeMax, step, msgs_before_step, nc, make([]int64, msgCount, msgCount), make([]int64, msgCount, msgCount), make([][]byte, msgCount, msgCount), make([]int64, msgCount, msgCount), new(int), new(int), sync.Mutex{}}
	*pingpong.last_rec_msg_id = -1
	nc.Subscribe(topic2, func(msg *nats.Msg) {
		pingpong.read_msg_time[*pingpong.n_received] = -time.Now().UnixNano()
		pingpong.msgs[*pingpong.n_received] = msg.Data
		pingpong.read_msg_time[*pingpong.n_received] += time.Now().UnixNano()
		pingpong.receive_timestamp[*pingpong.n_received] = time.Now().UnixNano()
		*pingpong.n_received += 1
	})
	return pingpong
}

func (pingpong TestPingPong) StartTestOld() int{
	isTimeoutEx := false

	time.Sleep(4*time.Second)

	for i := 0; i < pingpong.msgCount; i++ {
		if pingpong.isFirst {
			pingpong.publish(i, pingpong.msgSize)
		}

		isTimeoutEx = pingpong.wait_for_msg()
		if isTimeoutEx {
			break
		}
		if !pingpong.isFirst {
			pingpong.publish(i, pingpong.msgSize)
		}

		pingpong.toJson()

		if isTimeoutEx {
			return 2
		}
	}
	return 0
}

func (pingpong TestPingPong) wait_for_msg() bool{        //func waits for TIMEOUT to receive msgs
	start_timeout := time.Now().UnixNano()
	end_timeout := start_timeout

	notReceived := true
	for notReceived {

		pingpong.mu.Lock()      // mute thread to write msg and update _last_rec_msg_id

		if pingpong.receive() { // true - принято
			if !pingpong.isNew || !pingpong.isFirst {
				notReceived = false
			}
			*pingpong.last_rec_msg_id += 1
		} else {
			end_timeout = time.Now().UnixNano()
			if end_timeout - start_timeout > int64(timeout) {
				pingpong.mu.Unlock()
				return true
			}

		}
		pingpong.mu.Unlock()

		time.Sleep(time.Millisecond)
	}
	return false
}


func (pingpong TestPingPong) StartTestNew() int{
	future := make(chan bool)
	if pingpong.isFirst {   //run receiving msgs in another thread
		go func(pingpong TestPingPong, result chan bool) {
			result <- pingpong.wait_for_msg()
		}(pingpong, future)
	}
	time.Sleep(4*time.Second)
	cur_size := pingpong.msgSizeMin
	if pingpong.isFirst {
		for i := 0; i < pingpong.msgCount; i+=1 {
			if pingpong.msgSize == 0 {
				pingpong.mu.Lock()
				if (i - *pingpong.last_rec_msg_id > watermark) {
					i -= 1
					pingpong.mu.Unlock()
					continue;
				}
				pingpong.mu.Unlock()
			}

			if i % (pingpong.msgs_before_step - 1) == 0 && cur_size <= pingpong.msgSizeMax {
				cur_size += pingpong.step
			}

			pingpong.publish(i, cur_size)

			time.Sleep(time.Duration(pingpong.interval) * time.Millisecond)
		}
		_ = <-future
	} else{
		for !pingpong.wait_for_msg() {
			if *pingpong.last_rec_msg_id % (pingpong.msgs_before_step - 1) == 0 && cur_size <= pingpong.msgSizeMax {
				cur_size += pingpong.step
			}

			pingpong.publish(*pingpong.last_rec_msg_id, cur_size)

		}
	}

	pingpong.toJson()
	return 0
}



func (pingpong TestPingPong) StartTest() int {
	if pingpong.isNew {
		return pingpong.StartTestNew()
	}
	return pingpong.StartTestOld()
}

func (pingpong TestPingPong) toJson(){
	n := len(pingpong.msgs)
	info := make([]info, n, n)
	for i := 0; i<n; i++{
		json.Unmarshal(pingpong.msgs[i], &info[i].Msg)
		info[i].Msg.Read_proc_time = pingpong.read_msg_time[i]
		info[i].Msg.Proc_time = pingpong.write_msg_time[i]
		info[i].Msg.Receive_timestamp = pingpong.receive_timestamp[i]
		info[i].Msg.Delay = info[i].Msg.Receive_timestamp - info[i].Msg.Sent_time
	}
	out, err := json.Marshal(info)
	if err != nil{
		log.Fatal(err)
	}
	file, err := os.Create(pingpong.filename)
	if err != nil{
		log.Fatal(err)
	}
	defer file.Close()
	_, err = file.Write([]byte(out))
	if err != nil{
		log.Fatal(err)
	}
}

func (pingpong TestPingPong) receive() bool{
	count := *pingpong.n_received - *pingpong.rec_before
	if count > 0 {
		*pingpong.rec_before += 1
		return true
	}
	return false
}

func (pingpong TestPingPong) publish(id int, size int) {
	var str string = strings.Repeat("a", size)
	var msg msg
	msg.Sent_time = time.Now().UnixNano()
	msg.Id = id
	msg.Msg = str
	out, err := json.Marshal(msg)
        if err != nil {
                log.Fatal(err)
        }
	proc_time := time.Now().UnixNano()
	err = pingpong.nc.Publish(pingpong.topic1, out)
	pingpong.write_msg_time[id] = time.Now().UnixNano() - proc_time
        if err != nil {
                log.Fatal(err)
        }
}

func (pingpong TestPingPong) Close(){
	pingpong.nc.Close()
}
