const nodemailer = require('nodemailer');
const config_module = require("./config")

/**
 * 创建发送邮件的代理
 */
let transport = nodemailer.createTransport({
    host: 'smtp.163.com',
    port: 465,
    secure: true,
    auth: {
        user: config_module.email_user, // 发送方邮箱地址
        pass: config_module.email_pass // 邮箱授权码或者密码
    }
});

/**
 * 发送邮件的函数
 * @param {*} mailOptions_ 发送邮件的参数
 * @returns 
 */
function SendMail(mailOptions_){
    return new Promise(function(resolve, reject){//执行promise的条件就是括号中有一个函数被调用
        transport.sendMail(mailOptions_, function(error, info){
            if (error) {
                console.log(error);
                reject(error);
            } else {
                console.log('邮件已成功发送：' + info.response);
                resolve(info.response)
            }
        });
        //邮件发送后会有一个回调，这是异步进行的，但是我们想要它这个回调等待收到是否发送成功响应才调用
        //使用promise（类似c++中的future）将异步发送回调的机制转为同步（使用promise.wait()阻塞内部直接发送）

        //异步 = 操作不会等待前一个任务完成，可以几个任务交替来做，前一个任务没结束下一个任务就开始。
        //同步 = 操作按顺序执行，前一步等后一步完成再执行。等待某一步的结果后再继续执行

        //注意这里只是看起来像同步，实际上是异步操作的语法糖，不是真的同步，不会阻塞线程执行到await的时候会把这个异步操作交给底层，去做其他事情
        //等到异步任务完成，再回到await继续执行后续代码。同步是需要阻塞进程卡住等待结果。
    })
   
}

module.exports.SendMail = SendMail