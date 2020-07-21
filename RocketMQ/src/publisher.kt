import com.github.ajalt.clikt.output.TermUi.echo

import org.apache.rocketmq.client.producer.DefaultMQProducer
import org.apache.rocketmq.client.producer.SendCallback
import org.apache.rocketmq.client.producer.SendResult
import org.apache.rocketmq.common.message.Message
import org.apache.rocketmq.remoting.common.RemotingHelper
import java_interface.PublisherInterface

class Publisher(val topic: String, val msgCount: Int, val prior: Int, val cpu_index: Int,
                val min_msg_size: Int, val max_msg_size: Int, val step: Int, val interval: Int,
                val msgs_before_step: Int, val filename: String, val topic_priority: Int):
        PublisherInterface(topic, msgCount, prior, cpu_index, min_msg_size, max_msg_size, step, interval,
                msgs_before_step, filename, topic_priority) {
    val producer = DefaultMQProducer("publishers")
    init {
        producer.setNamesrvAddr("localhost:9876");
        producer.start()
        producer.retryTimesWhenSendAsyncFailed = 0
    }

    override fun publish(id: Int, size: Int): Long {
        val data = "a".padEnd(size, 'a')
        try {
            var curTime = System.nanoTime()
            val msg = Message("TopicTest",
                    "TagA",
                    "OrderID188",
                    "${id}ts:${curTime}data:$data".toByteArray(charset(RemotingHelper.DEFAULT_CHARSET)))
            producer.send(msg, object : SendCallback {
                override fun onSuccess(sendResult: SendResult) {
                    echo ("$id OK")
                }

                override fun onException(e: Throwable) {
                    echo ("$id Exception!")
                    e.printStackTrace()
                }
            })
            return curTime - System.nanoTime()
        } catch (e: Exception) {
            e.printStackTrace()
            return 0
        }
    }

}