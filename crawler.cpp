// ADD fetch time, success rate, fail rate

// later IP

#include <string>
#include <cstring>
#include <sstream>
#include <iostream>
#include <fstream>

#include <curlpp/cURLpp.hpp>
#include <curlpp/Easy.hpp>
#include <curlpp/Options.hpp>
#include <curlpp/Exception.hpp>

#include "rmTag.h"
#include "test_url.h"
#include "test_time.h"

#define MAX_FILE_LENGTH 50000
#define MAX_CRAWL_LIMIT 100

static std::fstream output_file;
static char *m_pBuffer = NULL;
static size_t m_Size = 0;
static test_time *tm = new test_time();
static test_url *tu = new test_url();

void *Realloc(void *ptr, size_t size)
{
    if (ptr)
        return realloc(ptr, size);
    else
        return malloc(size);
};

// Callback must be declared static, otherwise it won't link...
static size_t WriteMemoryCallback(char *ptr, size_t size, size_t nmemb)
{
    // Calculate the real size of the incoming buffer
    size_t realsize = size * nmemb;
    // (Re)Allocate memory for the buffer
    m_pBuffer = (char *)Realloc(m_pBuffer, m_Size + realsize);
    // Test if Buffer is initialized correctly & copy memory
    if (m_pBuffer == NULL)
    {
        realsize = 0;
    }
    //memset(&(m_pBuffer[m_Size]), '\0', realsize);
    memcpy(&(m_pBuffer[m_Size]), ptr, realsize);
    m_Size += realsize;
    // return the real size of the buffer...
    return realsize;
};

void write()
{
    output_file << "fetch time : " << tm->getCurrTime() << std::endl;
    output_file << "Size : " << m_Size << std::endl;
    output_file << "Content : " << std::endl
                << m_pBuffer << std::endl;
}

void retrieveBody()
{
    char body[4096];
    char *head = strstr(m_pBuffer, "<body");
    char *tail = strstr(m_pBuffer, "</body>"), *tmp = head + strlen("<body");
    int len = 0;
    while (1)
    {
        if (*tmp == '>')
            break;
        tmp++;
    }
    head = tmp++;
    while (1)
    {
        len++;
        if (strncmp(tmp, tail, 7) == 0)
            break;
        tmp++;
    }
    *tail = '\0';
    std::cout << head << std::endl;
    //memcpy(body, head, len+10);
    //strcpy(body, head);
    *tail = ' ';
    // std::cout << body << std::endl;
}

int main(int, char **) /* I/O for save data, using dataa batch to control I/O count */
{
    output_file.open("test.txt", std::fstream::out | std::fstream::app);

    int crawl_cnt=0;

    for (int i=0; i<MAX_CRAWL_LIMIT; i++)
    {
        std::string curr_url = tu->getTopUrl();
        try
        {
            // initial process
            curlpp::Cleanup myCleanup;
            curlpp::Easy myRequest;

            // create curl options
            curlpp::options::Url myUrl(curr_url);//std::string("https://star.ettoday.net/news/718757"));
            curlpp::types::WriteFunctionFunctor functor(WriteMemoryCallback);
            curlpp::options::WriteFunction *write_func = new curlpp::options::WriteFunction(functor);

            // bind options
            myRequest.setOpt(write_func);
            myRequest.setOpt(myUrl);
            // myRequest.setOpt(new curlpp::options::Verbose(true));

            // perform
            myRequest.perform();
        }
        catch (curlpp::LogicError &e)
        {
            /* 
             * writing failed crawled url info
             * format: md5 url reason
             */
            tu->addfailedUrl(curr_url, std::string(e.what()));
            continue;
        }
        catch (curlpp::RuntimeError &e)
        {
            /* 
             * writing failed crawled url info
             * format: md5 url reason
             */
            tu->addfailedUrl(curr_url, std::string(e.what()));
            continue;
        }
        if (m_Size == 0)
            return -1;
        std::cout << ++crawl_cnt << std::endl;
        std::cout << "rm script" << std::endl;
        rmTag(m_pBuffer, "script");
        std::cout << "rm style" << std::endl;
        rmTag(m_pBuffer, "style");
        std::cout << "write()" << std::endl;
        write();
        std::cout << "retrieve URL" << std::endl;
        tu->retrieveUrl(m_pBuffer);
        std::cout << "retrieve body" << std::endl;
        // retrieveBody();
        
    }
    output_file.close();
    std::cout << "delete tu" << std::endl;
    delete tu;
    std::cout << "delete tm" << std::endl;
    delete tm;
    free(m_pBuffer);
    return 0;
}